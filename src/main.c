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
#include <pspsysmem_kernel.h>
#include <psputilsforkernel.h>
#include <pspinit.h>
#include <string.h>
#include <stdio.h>
#include "lib.h"
#include "sctrl.h"

PSP_MODULE_INFO("ps1doc", 0x1000, 1, 0);
PSP_HEAP_SIZE_KB(0);

static STMOD_HANDLER previous = NULL;

SceIoStat stat;
char pathbuf[256], folder_id[64];
int doc_entry = 0;
u32 backup[2], patchaddr;

int (* sceMeAudio_0FA28FE6_)(u32 offset1, u32 offset2, int whence); //seek function
int (* sceMeAudio_2AB4FE43_)(u8 *buf, u32 size); //read/decrypt function
int sceMeAudio_2AB4FE43_hook(u8 *buf, u32 size)
{
	int ret = sceMeAudio_2AB4FE43_(buf, size);

	if ((size > 0x80) && (ret == (int)size) && !memcmp(buf, "\xFF\xFF\xFF\xFF", 4)) {
		sprintf(pathbuf, "ms0:/DOCS/PS1_%s", folder_id);
		sceIoMkdir(pathbuf, 0777);
		int i, j = _lw((u32)buf + 4);
		u32 ofs, sz;

		for (i = 0; i < j; i++) {
			ofs = _lw((u32)buf + (i << 7) + 8);
			sz = _lw((u32)buf + (i << 7) + 20);

			SceUID mem = sceKernelAllocPartitionMemory(2, "tmpbuf", PSP_SMEM_Low, sz, NULL);
			u8 *tmpbuf = (u8 *)sceKernelGetBlockHeadAddr(mem);

			sceMeAudio_0FA28FE6_(ofs, 0, 0);
			sceMeAudio_2AB4FE43_(tmpbuf, sz);

			sprintf(pathbuf, "ms0:/DOCS/PS1_%s/DOC_%03d.png", folder_id, i);

			while (memcmp(tmpbuf + sz - 8, "IEND\xAE\x42\x60\x82", 8))
				sz--;

			SceUID fd = sceIoOpen(pathbuf, 0x602, 0777);
			sceIoWrite(fd, tmpbuf, sz);
			sceIoClose(fd);

			sceKernelFreePartitionMemory(mem);
		}

		//done dumping, restore function.
		_sw(backup[0], patchaddr);
		_sw(backup[1], patchaddr + 4);
		ClearCaches();
	}

	return ret;
}

void patch_popsman()
{
	SceModule2 *mod = FindModuleByName("scePops_Manager");
	u32 addr;

	for (addr = mod->text_addr; addr < (mod->text_addr + mod->text_size); addr += 4) {
		if (_lw(addr) == 0x0080B021) { //move       $s6, $a0
			patchaddr = addr - 0x14;
			backup[0] = _lw(addr - 0x14);
			backup[1] = _lw(addr - 0x10);
			HIJACK_FUNCTION(addr - 0x14, sceMeAudio_2AB4FE43_hook, sceMeAudio_2AB4FE43_);
			ClearCaches();
		} else if (_lw(addr) == 0x2CC60001) { //sltiu      $a2, $a2, 1
			sceMeAudio_0FA28FE6_ = (void *)(addr - 0xC);
			break;
		}
	}

	sceIoMkdir("ms0:/DOCS", 0777);
}

int module_start_handler(SceModule2 *module)
{
	int ret = previous ? previous(module) : 0;

	if (!strcmp(module->modname, "pops"))
		patch_popsman();

	return ret;
}

int thread_start(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	previous = sctrlHENSetStartModuleHandler(module_start_handler);

	return 0;
}

int module_start(SceSize args, void *argp)
{
	strcpy(pathbuf, sceKernelInitFileName());
	int len = strlen(pathbuf);

	while (pathbuf[len] != '/')
		len--;

	pathbuf[len] = 0;

	while (pathbuf[len] != '/')
		len--;

	strcpy(folder_id, pathbuf + len + 1);

	SceUID thid = sceKernelCreateThread("ps1doc", thread_start, 0x22, 0x2000, 0, NULL);

	if (thid >= 0)
		sceKernelStartThread(thid, args, argp);

	return 0;
}

int module_stop(SceSize args __attribute__((unused)), void *argp __attribute__((unused)))
{
	return 0;
}
