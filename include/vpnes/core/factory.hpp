/**
 * @file
 *
 * Defines factories
 */
/*
 NES Emulator
 Copyright (C) 2012-2018  Ivanov Viktor

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#ifndef INCLUDE_VPNES_CORE_FACTORY_HPP_
#define INCLUDE_VPNES_CORE_FACTORY_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vpnes/vpnes.hpp>
#include <vpnes/core/frontend.hpp>
#include <vpnes/core/nes.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/gui/config.hpp>

namespace vpnes {

namespace core {

/**
 * NES factory
 *
 * @param appConfig Application config
 * @param config NES config
 * @return NES
 */
typedef CNES *(*NESFactory)(const SNESConfig &config, CFrontEnd *frontEnd);

namespace factory {

/**
 * NROM NES factory
 *
 * @param config NES config
 * @param frontEnd Front-end
 * @return NES
 */
CNES *factoryNROM(const SNESConfig &config, CFrontEnd *frontEnd);

}  // namespace factory

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_FACTORY_HPP_
