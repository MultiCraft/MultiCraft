#include <stdlib.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "ecrypt-sync.h"

#if LUA_VERSION_NUM == 501
#define l_setfuncs(L, funcs)    luaL_register(L, NULL, funcs)
#else
#define l_setfuncs(L, funcs)    luaL_setfuncs(L, funcs, 0)
#endif

static int chacha_generic_crypt(lua_State *L, bool is_ietf)
{
    ECRYPT_ctx ctx;
    const char *key, *iv, *plaintext, *counter;
    char *ciphertext;
    size_t keysize, ivsize, msglen, countersize;

    int rounds = luaL_checkinteger(L, 1);

    /* IETF only normalizes ChaCha 20. */
    if (rounds != 20 && (is_ietf || (rounds % 2 != 0)))
        return luaL_error(L, "invalid number of rounds: %d", rounds);

    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TSTRING);
    luaL_checktype(L, 4, LUA_TSTRING);

    key = lua_tolstring(L, 2, &keysize);
    iv = lua_tolstring(L, 3, &ivsize);
    plaintext = lua_tolstring(L, 4, &msglen);
    counter = luaL_optlstring(L, 5, NULL, &countersize);

    if (ivsize != (is_ietf ? 12 : 8))
        return luaL_error(L, "invalid IV size: %dB", (int)ivsize);

    if (keysize != 32 && (is_ietf || keysize != 16))
        return luaL_error(L, "invalid key size: %dB", (int)keysize);

    if (counter && countersize != (is_ietf ? 4 : 8))
        return luaL_error(L, "invalid counter size: %dB", (int)countersize);

    if (msglen == 0) { lua_pushlstring(L, "", 0); return 1; }

    ciphertext = malloc(msglen);
    if (!ciphertext) return luaL_error(L, "OOM");

    /* keysize and ivsize are in bits */
    ECRYPT_keysetup(&ctx, (u8*)key, 8 * keysize, 8 * ivsize);
    if (is_ietf) ECRYPT_IETF_ivsetup(&ctx, (u8*)iv, (u8*)counter);
    else ECRYPT_ivsetup(&ctx, (u8*)iv, (u8*)counter);
    ECRYPT_encrypt_bytes(&ctx, (u8*)plaintext, (u8*)ciphertext, msglen, rounds);

    lua_pushlstring(L, ciphertext, msglen);
    free(ciphertext);
    return 1;
}

static int chacha_ref_crypt(lua_State *L)
{ return chacha_generic_crypt(L, false); }

static int chacha_ietf_crypt(lua_State *L)
{ return chacha_generic_crypt(L, true); }

int luaopen_chacha(lua_State *L)
{
    struct luaL_Reg l[] = {
        { "ref_crypt", chacha_ref_crypt },
        { "ietf_crypt", chacha_ietf_crypt },
        { NULL, NULL }
    };
    lua_newtable(L);
    l_setfuncs(L, l);
    return 1;
}
