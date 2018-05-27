/**
 * @file
 *
 * Config gor GUI
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

#ifndef INCLUDE_VPNES_CONFIG_GUI_HPP_
#define INCLUDE_VPNES_CONFIG_GUI_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <string>
#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace config {

struct SGuiConfig {
  private:
    int m_width;
    int m_height;

  public:
    SGuiConfig() {
    }

    SGuiConfig(int width, int height) {
      m_width = width;
      m_height = height;
    }

    int getWidth() {
      return m_width;
    }

    int getHeight() {
      return m_height;
    }

    void setWidth(int width) {
      m_width = width;
    }

    void setHeight(int height) {
      m_height = height;
    }
};

}  // namespace config

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CONFIG_GUI_HPP_
