// SPDX-FileCopyrightText: 2024 Luanti Contributors
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <string>

namespace hashing
{

// Size of raw digest in bytes
constexpr size_t SHA1_DIGEST_SIZE = 20;

// Returns the digest of data
std::string sha1(const std::string &data);

}
