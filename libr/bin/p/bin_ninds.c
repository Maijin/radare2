/* radare - LGPL - 2015-2017 - a0rtega */

#include <r_types.h>
#include <r_util.h>
#include <r_lib.h>
#include <r_bin.h>
#include <string.h>

#include "../format/nin/nds.h"

static struct nds_hdr loaded_header;

static bool check_bytes(const ut8 *buf, ut64 length) {
	ut8 ninlogohead[6];
	if (!buf || length < sizeof(struct nds_hdr)) { /* header size */
		return false;
	}
	memcpy (ninlogohead, buf + 0xc0, 6);
	/* begin of nintendo logo =    \x24\xff\xae\x51\x69\x9a */
	return (!memcmp (ninlogohead, "\x24\xff\xae\x51\x69\x9a", 6))? true: false;
}

static bool check(RBinFile *arch) {
	const ut8 *bytes = arch? r_buf_buffer (arch->buf): NULL;
	ut64 sz = arch? r_buf_size (arch->buf): 0;
	return check_bytes (bytes, sz);
}

static void *load_bytes(RBinFile *arch, const ut8 *buf, ut64 sz, ut64 loadaddr, Sdb *sdb) {
	return memcpy (&loaded_header, buf, sizeof(struct nds_hdr));
}

static bool load(RBinFile *arch) {
	const ut8 *bytes = arch? r_buf_buffer (arch->buf): NULL;
	ut64 sz = arch? r_buf_size (arch->buf): 0;
	if (!arch || !arch->o) {
		return false;
	}
	arch->o->bin_obj = load_bytes (arch, bytes, sz, arch->o->loadaddr, arch->sdb);
	return check_bytes (bytes, sz);
}

static int destroy(RBinFile *arch) {
	r_buf_free (arch->buf);
	arch->buf = NULL;
	return true;
}

static ut64 baddr(RBinFile *arch) {
	return (ut64) loaded_header.arm9_ram_address;
}

static ut64 boffset(RBinFile *arch) {
	return 0LL;
}

static RList *sections(RBinFile *arch) {
	RList *ret = NULL;
	RBinSection *ptr9 = NULL, *ptr7 = NULL;

	if (!(ret = r_list_new ())) {
		return NULL;
	}
	if (!(ptr9 = R_NEW0 (RBinSection))) {
		r_list_free (ret);
		return NULL;
	}
	if (!(ptr7 = R_NEW0 (RBinSection))) {
		r_list_free (ret);
		free (ptr9);
		return NULL;
	}

	strncpy (ptr9->name, "arm9", 5);
	ptr9->size = loaded_header.arm9_size;
	ptr9->vsize = loaded_header.arm9_size;
	ptr9->paddr = loaded_header.arm9_rom_offset;
	ptr9->vaddr = loaded_header.arm9_ram_address;
	ptr9->srwx = r_str_rwx ("mrwx");
	ptr9->add = true;
	r_list_append (ret, ptr9);

	strncpy (ptr7->name, "arm7", 5);
	ptr7->size = loaded_header.arm7_size;
	ptr7->vsize = loaded_header.arm7_size;
	ptr7->paddr = loaded_header.arm7_rom_offset;
	ptr7->vaddr = loaded_header.arm7_ram_address;
	ptr7->srwx = r_str_rwx ("mrwx");
	ptr7->add = true;
	r_list_append (ret, ptr7);

	return ret;
}

static RList *entries(RBinFile *arch) {
	RList *ret = r_list_new ();
	RBinAddr *ptr9 = NULL, *ptr7 = NULL;

	if (arch && arch->buf) {
		if (!ret) {
			return NULL;
		}
		ret->free = free;
		if (!(ptr9 = R_NEW0 (RBinAddr))) {
			r_list_free (ret);
			return NULL;
		}
		if (!(ptr7 = R_NEW0 (RBinAddr))) {
			r_list_free (ret);
			free (ptr9);
			return NULL;
		}

		/* ARM9 entry point */
		ptr9->vaddr = loaded_header.arm9_entry_address;
		// ptr9->paddr = loaded_header.arm9_entry_address;
		r_list_append (ret, ptr9);

		/* ARM7 entry point */
		ptr7->vaddr = loaded_header.arm7_entry_address;
		// ptr7->paddr = loaded_header.arm7_entry_address;
		r_list_append (ret, ptr7);
	}
	return ret;
}

static RBinInfo *info(RBinFile *arch) {
	char filepath[1024];
	RBinInfo *ret = R_NEW0 (RBinInfo);
	if (!ret) {
		return NULL;
	}

	if (!arch || !arch->buf) {
		free (ret);
		return NULL;
	}

	strncpy (filepath, (char *) loaded_header.title, 0xC);
	strncat (filepath, " - ", 3);
	strncat (filepath, (char *) loaded_header.gamecode, 0x4);

	ret->file = strdup (filepath);
	ret->type = strdup ("ROM");
	ret->machine = strdup ("Nintendo DS");
	ret->os = strdup ("nds");
	ret->arch = strdup ("arm");
	ret->has_va = true;
	ret->bits = 32;

	return ret;
}

RBinPlugin r_bin_plugin_ninds = {
	.name = "ninds",
	.desc = "Nintendo DS format r_bin plugin",
	.license = "LGPL3",
	.load = &load,
	.load_bytes = &load_bytes,
	.destroy = &destroy,
	.check = &check,
	.check_bytes = &check_bytes,
	.baddr = &baddr,
	.boffset = &boffset,
	.entries = &entries,
	.sections = &sections,
	.info = &info,
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_BIN,
	.data = &r_bin_plugin_ninds,
	.version = R2_VERSION
};
#endif

