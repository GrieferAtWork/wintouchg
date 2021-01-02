# Copyright (c) 2020-2021 Griefer@Work
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgement (see the following) in the product
#    documentation is required:
#    Portions Copyright (c) 2020-2021 Griefer@Work
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
from PIL import Image

print("/* Copyright (c) 2020-2021 Griefer@Work                                       *")
print(" *                                                                            *")
print(" * This software is provided 'as-is', without any express or implied          *")
print(" * warranty. In no event will the authors be held liable for any damages      *")
print(" * arising from the use of this software.                                     *")
print(" *                                                                            *")
print(" * Permission is granted to anyone to use this software for any purpose,      *")
print(" * including commercial applications, and to alter it and redistribute it     *")
print(" * freely, subject to the following restrictions:                             *")
print(" *                                                                            *")
print(" * 1. The origin of this software must not be misrepresented; you must not    *")
print(" *    claim that you wrote the original software. If you use this software    *")
print(" *    in a product, an acknowledgement (see the following) in the product     *")
print(" *    documentation is required:                                              *")
print(" *    Portions Copyright (c) 2020-2021 Griefer@Work                           *")
print(" * 2. Altered source versions must be plainly marked as such, and must not be *")
print(" *    misrepresented as being the original software.                          *")
print(" * 3. This notice may not be removed or altered from any source distribution. *")
print(" */")
print('#ifndef GUARD_WINTOUCHG_GUI_GUI_C')
print('#define GUARD_WINTOUCHG_GUI_GUI_C 1')
print('')
print('#include <stdint.h>')
print('')
print('#ifdef __cplusplus')
print('extern "C" {')
print('#endif /* __cplusplus */')
print('')

img = Image.open('gui.png').convert('RGBA')
AREAS = [('CONFIG_WITHOUT_CP', 'cp_l',    (0,   0,  32, 1)),
         ('CONFIG_WITHOUT_CP', 'cp_c',    (33,  0,  1,  1)),
         ('CONFIG_WITHOUT_CP', 'cp_r',    (35,  0,  32, 1)),
         ('CONFIG_WITHOUT_CP', 'cp_bl',   (0,   2,  32, 32)),
         ('CONFIG_WITHOUT_CP', 'cp_b',    (33,  2,  1,  32)),
         ('CONFIG_WITHOUT_CP', 'cp_br',   (35,  2,  32, 32)),
         ('CONFIG_WITHOUT_CP', 'cp_wifi', (0,   35, 64, 64)),
         ('CONFIG_WITHOUT_CP', 'cp_bth',  (65,  35, 32, 64)),
         ('CONFIG_WITHOUT_CP', 'cp_nlck', (98,  35, 64, 64)),
         ('CONFIG_WITHOUT_CP', 'cp_rlck', (163, 35, 64, 64)),
         ]
print("#if 0")
prevCond = None
for a in AREAS:
	if a[0] != prevCond:
		if prevCond is not None:
			print("#endif /* !" + prevCond + " */")
		print("#ifndef " + a[0])
		prevCond = a[0]
	name = "gui_" + a[1]
	area = a[2]
	print("extern uint8_t const " + name + "[" + str(area[2]) + "][" + str(area[3]) + "][4];")
if prevCond is not None:
	print("#endif /* !" + prevCond + " */")
print("#endif")
print("")
def hex2(x):
	return "0x" + hex(x)[2:].zfill(2)

prevCond = None
for a in AREAS:
	if a[0] != prevCond:
		if prevCond is not None:
			print("#endif /* !" + prevCond + " */")
		print("#ifndef " + a[0])
		prevCond = a[0]
	name = "gui_" + a[1]
	area = a[2]
	bounds = (area[0], area[1], area[0] + area[2], area[1] + area[3])
	data = list(img.crop(bounds).getdata())
	cnt  = area[2] * area[3]
	print("uint8_t const " + name + "[" + str(cnt) + "][4] = {")
	i = 0
	for e in data:
		if (i % 4) == 0:
			print("\t", end = "")
		else:
			print(" ", end = "")
		if e[3] == 0:
			e = (0,0,0,0)
		print("{" +
		      hex2(e[2]) + "," +
		      hex2(e[1]) + "," +
		      hex2(e[0]) + "," +
		      hex2(e[3]) +
		      "}", end = "")
		if i != cnt - 1:
			print(",", end = "")
		if (i % 4) == 3:
			print("")
		i = i + 1
	print("};")
if prevCond is not None:
	print("#endif /* !" + prevCond + " */")
print('')
print('#ifdef __cplusplus')
print('} /* extern "C" */')
print('#endif /* __cplusplus */')
print('')
print('#endif /* !GUARD_WINTOUCHG_GUI_GUI_C */')
