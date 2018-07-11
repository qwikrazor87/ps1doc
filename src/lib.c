/*
 *  Copyright (C) 2018 qwikrazor87
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pspkernel.h>
#include <string.h>
#include "lib.h"

void ClearCaches(void)
{
	sceKernelDcacheWritebackInvalidateAll();
}

SceModule2 *FindModuleByName(const char *module)
{
	u32 kaddr;
	SceModule2 *mod = (SceModule2 *)0;

	//search for base module struct, then iterate from there.
	for (kaddr = 0x88000000; kaddr < 0x88300000; kaddr += 4) {
		if (!strcmp((const char *)kaddr, "sceSystemMemoryManager")) {
			if ((*(u32*)(kaddr + 0x64) == *(u32*)(kaddr + 0x78)) && \
				(*(u32*)(kaddr + 0x68) == *(u32*)(kaddr + 0x88))) {
				if (*(u32*)(kaddr + 0x64) && *(u32*)(kaddr + 0x68)) {
					mod = (SceModule2 *)(kaddr - 8);
					break;
				}
			}
		}
	}

	while (mod) {
		if (!strcmp(mod->modname, module))
			break;

		mod = mod->next;
	}

	return mod;
}

