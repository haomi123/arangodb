////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2023 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Manuel Baesler
////////////////////////////////////////////////////////////////////////////////

#include "Auth/Common.h"
#include "Basics/Exceptions.h"

using namespace arangodb;

static_assert(auth::Level::UNDEFINED < auth::Level::NONE, "undefined < none");
static_assert(auth::Level::NONE < auth::Level::RO, "none < ro");
static_assert(auth::Level::RO < auth::Level::RW, "none < ro");

auth::Level arangodb::auth::convertToAuthLevel(velocypack::Slice grants) {
  return convertToAuthLevel(grants.stringView());
}

auth::Level arangodb::auth::convertToAuthLevel(std::string_view grants) {
  if (grants == "rw") {
    return auth::Level::RW;
  } else if (grants == "ro") {
    return auth::Level::RO;
  } else if (grants == "none" || grants.empty()) {
    return auth::Level::NONE;
  }
  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                 "expecting access type 'rw', 'ro' or 'none'");
}

std::string_view arangodb::auth::convertFromAuthLevel(auth::Level lvl) {
  if (lvl == auth::Level::RW) {
    return "rw";
  } else if (lvl == auth::Level::RO) {
    return "ro";
  } else if (lvl == auth::Level::NONE) {
    return "none";
  } else {
    return "undefined";
  }
}
