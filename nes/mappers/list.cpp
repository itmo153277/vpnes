/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

\****************************************************************************/

#include "list.h"
#include "nrom.h"

namespace vpnes {

namespace mappers {

/* Обработчик по умолчанию */
CNESConfig *DefaultHandler(ines::NES_ROM_Data *Data) {
	return NULL;
}

/* Список обработчиков маппреов */
const MapperHandler MapperHandlerList[ines::MappersCount] = {
	NROMHandler,   /* NROM */
	DefaultHandler /* UNKNOWN */
};

/* Список мапперов по номерам */
const ines::MapperID MapperIDList[256] = {
	ines::NROM,    /* 0 */ 
	ines::UNKNOWN, /* 1 */ 
	ines::UNKNOWN, /* 2 */ 
	ines::UNKNOWN, /* 3 */ 
	ines::UNKNOWN, /* 4 */ 
	ines::UNKNOWN, /* 5 */ 
	ines::UNKNOWN, /* 6 */ 
	ines::UNKNOWN, /* 7 */ 
	ines::UNKNOWN, /* 8 */ 
	ines::UNKNOWN, /* 9 */ 
	ines::UNKNOWN, /* 10 */ 
	ines::UNKNOWN, /* 11 */ 
	ines::UNKNOWN, /* 12 */ 
	ines::UNKNOWN, /* 13 */ 
	ines::UNKNOWN, /* 14 */ 
	ines::UNKNOWN, /* 15 */ 
	ines::UNKNOWN, /* 16 */ 
	ines::UNKNOWN, /* 17 */ 
	ines::UNKNOWN, /* 18 */ 
	ines::UNKNOWN, /* 19 */ 
	ines::UNKNOWN, /* 20 */ 
	ines::UNKNOWN, /* 21 */ 
	ines::UNKNOWN, /* 22 */ 
	ines::UNKNOWN, /* 23 */ 
	ines::UNKNOWN, /* 24 */ 
	ines::UNKNOWN, /* 25 */ 
	ines::UNKNOWN, /* 26 */ 
	ines::UNKNOWN, /* 27 */ 
	ines::UNKNOWN, /* 28 */ 
	ines::UNKNOWN, /* 29 */ 
	ines::UNKNOWN, /* 30 */ 
	ines::UNKNOWN, /* 31 */ 
	ines::UNKNOWN, /* 32 */ 
	ines::UNKNOWN, /* 33 */ 
	ines::UNKNOWN, /* 34 */ 
	ines::UNKNOWN, /* 35 */ 
	ines::UNKNOWN, /* 36 */ 
	ines::UNKNOWN, /* 37 */ 
	ines::UNKNOWN, /* 38 */ 
	ines::UNKNOWN, /* 39 */ 
	ines::UNKNOWN, /* 40 */ 
	ines::UNKNOWN, /* 41 */ 
	ines::UNKNOWN, /* 42 */ 
	ines::UNKNOWN, /* 43 */ 
	ines::UNKNOWN, /* 44 */ 
	ines::UNKNOWN, /* 45 */ 
	ines::UNKNOWN, /* 46 */ 
	ines::UNKNOWN, /* 47 */ 
	ines::UNKNOWN, /* 48 */ 
	ines::UNKNOWN, /* 49 */ 
	ines::UNKNOWN, /* 50 */ 
	ines::UNKNOWN, /* 51 */ 
	ines::UNKNOWN, /* 52 */ 
	ines::UNKNOWN, /* 53 */ 
	ines::UNKNOWN, /* 54 */ 
	ines::UNKNOWN, /* 55 */ 
	ines::UNKNOWN, /* 56 */ 
	ines::UNKNOWN, /* 57 */ 
	ines::UNKNOWN, /* 58 */ 
	ines::UNKNOWN, /* 59 */ 
	ines::UNKNOWN, /* 60 */ 
	ines::UNKNOWN, /* 61 */ 
	ines::UNKNOWN, /* 62 */ 
	ines::UNKNOWN, /* 63 */ 
	ines::UNKNOWN, /* 64 */ 
	ines::UNKNOWN, /* 65 */ 
	ines::UNKNOWN, /* 66 */ 
	ines::UNKNOWN, /* 67 */ 
	ines::UNKNOWN, /* 68 */ 
	ines::UNKNOWN, /* 69 */ 
	ines::UNKNOWN, /* 70 */ 
	ines::UNKNOWN, /* 71 */ 
	ines::UNKNOWN, /* 72 */ 
	ines::UNKNOWN, /* 73 */ 
	ines::UNKNOWN, /* 74 */ 
	ines::UNKNOWN, /* 75 */ 
	ines::UNKNOWN, /* 76 */ 
	ines::UNKNOWN, /* 77 */ 
	ines::UNKNOWN, /* 78 */ 
	ines::UNKNOWN, /* 79 */ 
	ines::UNKNOWN, /* 80 */ 
	ines::UNKNOWN, /* 81 */ 
	ines::UNKNOWN, /* 82 */ 
	ines::UNKNOWN, /* 83 */ 
	ines::UNKNOWN, /* 84 */ 
	ines::UNKNOWN, /* 85 */ 
	ines::UNKNOWN, /* 86 */ 
	ines::UNKNOWN, /* 87 */ 
	ines::UNKNOWN, /* 88 */ 
	ines::UNKNOWN, /* 89 */ 
	ines::UNKNOWN, /* 90 */ 
	ines::UNKNOWN, /* 91 */ 
	ines::UNKNOWN, /* 92 */ 
	ines::UNKNOWN, /* 93 */ 
	ines::UNKNOWN, /* 94 */ 
	ines::UNKNOWN, /* 95 */ 
	ines::UNKNOWN, /* 96 */ 
	ines::UNKNOWN, /* 97 */ 
	ines::UNKNOWN, /* 98 */ 
	ines::UNKNOWN, /* 99 */ 
	ines::UNKNOWN, /* 100 */ 
	ines::UNKNOWN, /* 101 */ 
	ines::UNKNOWN, /* 102 */ 
	ines::UNKNOWN, /* 103 */ 
	ines::UNKNOWN, /* 104 */ 
	ines::UNKNOWN, /* 105 */ 
	ines::UNKNOWN, /* 106 */ 
	ines::UNKNOWN, /* 107 */ 
	ines::UNKNOWN, /* 108 */ 
	ines::UNKNOWN, /* 109 */ 
	ines::UNKNOWN, /* 110 */ 
	ines::UNKNOWN, /* 111 */ 
	ines::UNKNOWN, /* 112 */ 
	ines::UNKNOWN, /* 113 */ 
	ines::UNKNOWN, /* 114 */ 
	ines::UNKNOWN, /* 115 */ 
	ines::UNKNOWN, /* 116 */ 
	ines::UNKNOWN, /* 117 */ 
	ines::UNKNOWN, /* 118 */ 
	ines::UNKNOWN, /* 119 */ 
	ines::UNKNOWN, /* 120 */ 
	ines::UNKNOWN, /* 121 */ 
	ines::UNKNOWN, /* 122 */ 
	ines::UNKNOWN, /* 123 */ 
	ines::UNKNOWN, /* 124 */ 
	ines::UNKNOWN, /* 125 */ 
	ines::UNKNOWN, /* 126 */ 
	ines::UNKNOWN, /* 127 */ 
	ines::UNKNOWN, /* 128 */ 
	ines::UNKNOWN, /* 129 */ 
	ines::UNKNOWN, /* 130 */ 
	ines::UNKNOWN, /* 131 */ 
	ines::UNKNOWN, /* 132 */ 
	ines::UNKNOWN, /* 133 */ 
	ines::UNKNOWN, /* 134 */ 
	ines::UNKNOWN, /* 135 */ 
	ines::UNKNOWN, /* 136 */ 
	ines::UNKNOWN, /* 137 */ 
	ines::UNKNOWN, /* 138 */ 
	ines::UNKNOWN, /* 139 */ 
	ines::UNKNOWN, /* 140 */ 
	ines::UNKNOWN, /* 141 */ 
	ines::UNKNOWN, /* 142 */ 
	ines::UNKNOWN, /* 143 */ 
	ines::UNKNOWN, /* 144 */ 
	ines::UNKNOWN, /* 145 */ 
	ines::UNKNOWN, /* 146 */ 
	ines::UNKNOWN, /* 147 */ 
	ines::UNKNOWN, /* 148 */ 
	ines::UNKNOWN, /* 149 */ 
	ines::UNKNOWN, /* 150 */ 
	ines::UNKNOWN, /* 151 */ 
	ines::UNKNOWN, /* 152 */ 
	ines::UNKNOWN, /* 153 */ 
	ines::UNKNOWN, /* 154 */ 
	ines::UNKNOWN, /* 155 */ 
	ines::UNKNOWN, /* 156 */ 
	ines::UNKNOWN, /* 157 */ 
	ines::UNKNOWN, /* 158 */ 
	ines::UNKNOWN, /* 159 */ 
	ines::UNKNOWN, /* 160 */ 
	ines::UNKNOWN, /* 161 */ 
	ines::UNKNOWN, /* 162 */ 
	ines::UNKNOWN, /* 163 */ 
	ines::UNKNOWN, /* 164 */ 
	ines::UNKNOWN, /* 165 */ 
	ines::UNKNOWN, /* 166 */ 
	ines::UNKNOWN, /* 167 */ 
	ines::UNKNOWN, /* 168 */ 
	ines::UNKNOWN, /* 169 */ 
	ines::UNKNOWN, /* 170 */ 
	ines::UNKNOWN, /* 171 */ 
	ines::UNKNOWN, /* 172 */ 
	ines::UNKNOWN, /* 173 */ 
	ines::UNKNOWN, /* 174 */ 
	ines::UNKNOWN, /* 175 */ 
	ines::UNKNOWN, /* 176 */ 
	ines::UNKNOWN, /* 177 */ 
	ines::UNKNOWN, /* 178 */ 
	ines::UNKNOWN, /* 179 */ 
	ines::UNKNOWN, /* 180 */ 
	ines::UNKNOWN, /* 181 */ 
	ines::UNKNOWN, /* 182 */ 
	ines::UNKNOWN, /* 183 */ 
	ines::UNKNOWN, /* 184 */ 
	ines::UNKNOWN, /* 185 */ 
	ines::UNKNOWN, /* 186 */ 
	ines::UNKNOWN, /* 187 */ 
	ines::UNKNOWN, /* 188 */ 
	ines::UNKNOWN, /* 189 */ 
	ines::UNKNOWN, /* 190 */ 
	ines::UNKNOWN, /* 191 */ 
	ines::UNKNOWN, /* 192 */ 
	ines::UNKNOWN, /* 193 */ 
	ines::UNKNOWN, /* 194 */ 
	ines::UNKNOWN, /* 195 */ 
	ines::UNKNOWN, /* 196 */ 
	ines::UNKNOWN, /* 197 */ 
	ines::UNKNOWN, /* 198 */ 
	ines::UNKNOWN, /* 199 */ 
	ines::UNKNOWN, /* 200 */ 
	ines::UNKNOWN, /* 201 */ 
	ines::UNKNOWN, /* 202 */ 
	ines::UNKNOWN, /* 203 */ 
	ines::UNKNOWN, /* 204 */ 
	ines::UNKNOWN, /* 205 */ 
	ines::UNKNOWN, /* 206 */ 
	ines::UNKNOWN, /* 207 */ 
	ines::UNKNOWN, /* 208 */ 
	ines::UNKNOWN, /* 209 */ 
	ines::UNKNOWN, /* 210 */ 
	ines::UNKNOWN, /* 211 */ 
	ines::UNKNOWN, /* 212 */ 
	ines::UNKNOWN, /* 213 */ 
	ines::UNKNOWN, /* 214 */ 
	ines::UNKNOWN, /* 215 */ 
	ines::UNKNOWN, /* 216 */ 
	ines::UNKNOWN, /* 217 */ 
	ines::UNKNOWN, /* 218 */ 
	ines::UNKNOWN, /* 219 */ 
	ines::UNKNOWN, /* 220 */ 
	ines::UNKNOWN, /* 221 */ 
	ines::UNKNOWN, /* 222 */ 
	ines::UNKNOWN, /* 223 */ 
	ines::UNKNOWN, /* 224 */ 
	ines::UNKNOWN, /* 225 */ 
	ines::UNKNOWN, /* 226 */ 
	ines::UNKNOWN, /* 227 */ 
	ines::UNKNOWN, /* 228 */ 
	ines::UNKNOWN, /* 229 */ 
	ines::UNKNOWN, /* 230 */ 
	ines::UNKNOWN, /* 231 */ 
	ines::UNKNOWN, /* 232 */ 
	ines::UNKNOWN, /* 233 */ 
	ines::UNKNOWN, /* 234 */ 
	ines::UNKNOWN, /* 235 */ 
	ines::UNKNOWN, /* 236 */ 
	ines::UNKNOWN, /* 237 */ 
	ines::UNKNOWN, /* 238 */ 
	ines::UNKNOWN, /* 239 */ 
	ines::UNKNOWN, /* 240 */ 
	ines::UNKNOWN, /* 241 */ 
	ines::UNKNOWN, /* 242 */ 
	ines::UNKNOWN, /* 243 */ 
	ines::UNKNOWN, /* 244 */ 
	ines::UNKNOWN, /* 245 */ 
	ines::UNKNOWN, /* 246 */ 
	ines::UNKNOWN, /* 247 */ 
	ines::UNKNOWN, /* 248 */ 
	ines::UNKNOWN, /* 249 */ 
	ines::UNKNOWN, /* 250 */ 
	ines::UNKNOWN, /* 251 */ 
	ines::UNKNOWN, /* 252 */ 
	ines::UNKNOWN, /* 253 */ 
	ines::UNKNOWN, /* 254 */ 
	ines::UNKNOWN  /* 255 */ 
};

}

}
