#!/usr/bin/env python3
"""
Застосовує всі правки для дампу назв текстур у файл при стрибку.
Запускати з кореня клонованого репозиторію MultiCraft:
    python3 apply_texture_dump.py
"""
import sys

def patch(path, replacements):
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    for old, new, label in replacements:
        if old not in content:
            print(f"[ПОМИЛКА] Не знайдено анкер '{label}' у {path}")
            print("          Файл міг змінитись — пришли його вміст ще раз.")
            sys.exit(1)
        if new in content and old != new:
            print(f"[ПРОПУЩЕНО] '{label}' у {path} — схоже, вже застосовано.")
            continue
        content = content.replace(old, new, 1)
        print(f"[OK] Застосовано '{label}' у {path}")

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


# ---------------------------------------------------------------------------
# 1) src/client/clientmedia.h
# ---------------------------------------------------------------------------
patch("src/client/clientmedia.h", [
    (
        "\tbool isStarted() const {\n"
        "\t\treturn m_initial_step_done;\n"
        "\t}\n",

        "\tbool isStarted() const {\n"
        "\t\treturn m_initial_step_done;\n"
        "\t}\n\n"
        "\t// Повертає імена всіх файлів медіа, оголошених сервером\n"
        "\t// (використовується для дампу списку текстур)\n"
        "\tstd::vector<std::string> getFileNames() const\n"
        "\t{\n"
        "\t\tstd::vector<std::string> names;\n"
        "\t\tnames.reserve(m_files.size());\n"
        "\t\tfor (const auto &it : m_files)\n"
        "\t\t\tnames.push_back(it.first);\n"
        "\t\treturn names;\n"
        "\t}\n",

        "getFileNames() у clientmedia.h",
    ),
])

# ---------------------------------------------------------------------------
# 2) src/client/client.h
# ---------------------------------------------------------------------------
patch("src/client/client.h", [
    (
        "{ return !m_media_downloader; }\n",

        "{ return !m_media_downloader; }\n\n"
        "\tconst std::vector<std::string> &getAllMediaNames() const\n"
        "\t{ return m_all_media_names; }\n",

        "getAllMediaNames() у client.h",
    ),
    (
        "\tClientMediaDownloader *m_media_downloader;\n",

        "\tstd::vector<std::string> m_all_media_names;\n"
        "\tClientMediaDownloader *m_media_downloader;\n",

        "m_all_media_names член у client.h",
    ),
])

# ---------------------------------------------------------------------------
# 3) src/client/client.cpp
# ---------------------------------------------------------------------------
patch("src/client/client.cpp", [
    (
        "\tif (m_media_downloader && m_media_downloader->isStarted()) {\n"
        "\t\tm_media_downloader->step(this);\n"
        "\t\tif (m_media_downloader->isDone()) {\n"
        "\t\t\tdelete m_media_downloader;\n"
        "\t\t\tm_media_downloader = NULL;\n"
        "\t\t}\n"
        "\t}\n",

        "\tif (m_media_downloader && m_media_downloader->isStarted()) {\n"
        "\t\tm_media_downloader->step(this);\n"
        "\t\tif (m_media_downloader->isDone()) {\n"
        "\t\t\tm_all_media_names = m_media_downloader->getFileNames();\n"
        "\t\t\tdelete m_media_downloader;\n"
        "\t\t\tm_media_downloader = NULL;\n"
        "\t\t}\n"
        "\t}\n",

        "збереження m_all_media_names у client.cpp",
    ),
])

# ---------------------------------------------------------------------------
# 4) src/client/game.cpp
# ---------------------------------------------------------------------------
patch("src/client/game.cpp", [
    (
        "\t} else if (wasKeyDown(KeyType::JUMP)) {\n"
        "#ifdef HAVE_TOUCHSCREENGUI\n"
        "\t\tif (isKeyDown(KeyType::SNEAK) && client->checkPrivilege(\"fly\"))\n"
        "\t\t\ttoggleFast();\n"
        "\t\telse\n"
        "#endif\n"
        "\t\ttoggleFreeMoveAlt();\n",

        "\t} else if (wasKeyDown(KeyType::JUMP)) {\n"
        "\t\tif (!texture_list_dumped) {\n"
        "\t\t\ttexture_list_dumped = true;\n"
        "\t\t\tstd::string path = porting::path_user + DIR_DELIM + \"all_textures.txt\";\n"
        "\t\t\tstd::ofstream file(path.c_str());\n"
        "\t\t\tif (file.is_open()) {\n"
        "\t\t\t\tint count = 0;\n"
        "\t\t\t\tfor (const std::string &name : client->getAllMediaNames()) {\n"
        "\t\t\t\t\tif (str_ends_with(name, \".png\") ||\n"
        "\t\t\t\t\t\t\tstr_ends_with(name, \".jpg\") ||\n"
        "\t\t\t\t\t\t\tstr_ends_with(name, \".jpeg\") ||\n"
        "\t\t\t\t\t\t\tstr_ends_with(name, \".tga\") ||\n"
        "\t\t\t\t\t\t\tstr_ends_with(name, \".bmp\")) {\n"
        "\t\t\t\t\t\tfile << name << \"\\n\";\n"
        "\t\t\t\t\t\tcount++;\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t}\n"
        "\t\t\t\tfile.close();\n"
        "\t\t\t\tm_game_ui->showStatusText(utf8_to_wide(\n"
        "\t\t\t\t\t\t\"Saved \" + std::to_string(count) +\n"
        "\t\t\t\t\t\t\" texture names to \" + path));\n"
        "\t\t\t}\n"
        "\t\t}\n"
        "#ifdef HAVE_TOUCHSCREENGUI\n"
        "\t\tif (isKeyDown(KeyType::SNEAK) && client->checkPrivilege(\"fly\"))\n"
        "\t\t\ttoggleFast();\n"
        "\t\telse\n"
        "#endif\n"
        "\t\ttoggleFreeMoveAlt();\n",

        "дамп текстур у JUMP-обробнику в game.cpp",
    ),
    (
        "\tClient *client = nullptr;\n",

        "\tClient *client = nullptr;\n"
        "\tbool texture_list_dumped = false;\n",

        "texture_list_dumped член у game.cpp (class Game)",
    ),
])

# ---------------------------------------------------------------------------
# 5) додаємо #include <fstream> у game.cpp, якщо його ще нема
# ---------------------------------------------------------------------------
with open("src/client/game.cpp", "r", encoding="utf-8") as f:
    content = f.read()

if "#include <fstream>" not in content:
    lines = content.split("\n")
    for i, line in enumerate(lines):
        if line.startswith("#include"):
            lines.insert(i, "#include <fstream>")
            break
    content = "\n".join(lines)
    with open("src/client/game.cpp", "w", encoding="utf-8") as f:
        f.write(content)
    print("[OK] Додано #include <fstream> у game.cpp")
else:
    print("[ПРОПУЩЕНО] #include <fstream> вже є у game.cpp")

print("\nГотово! Тепер збери проєкт (git diff щоб перевірити зміни).")
