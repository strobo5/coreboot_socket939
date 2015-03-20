/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <stdlib.h>
#include <cbmem.h>
#include <console/console.h>
#include <fallback.h>
#include <lib.h>
#include <program_loading.h>
#include <timestamp.h>

extern const struct payload_loader_ops vboot_payload_loader;
extern const struct payload_loader_ops cbfs_payload_loader;

static const struct payload_loader_ops *payload_ops[] = {
#if CONFIG_VBOOT_VERIFY_FIRMWARE
	&vboot_payload_loader,
#endif
	&cbfs_payload_loader,
};

static struct payload global_payload = {
	.prog = {
		.name = CONFIG_CBFS_PREFIX "/payload",
		.type = PROG_PAYLOAD,
	},
};

void __attribute__((weak)) mirror_payload(struct payload *payload)
{
	return;
}

void payload_load(void)
{
	int i;
	const struct payload_loader_ops *ops;
	struct payload *payload = &global_payload;
	struct prog * prog = &payload->prog;

	for (i = 0; i < ARRAY_SIZE(payload_ops); i++) {
		ops = payload_ops[i];
		if (ops->locate(payload) < 0) {
			printk(BIOS_DEBUG, "%s: could not locate payload.\n",
				ops->name);
			continue;
		}
		printk(BIOS_DEBUG, "%s: located payload @ %p, %zu bytes.\n",
			ops->name, prog_start(prog), prog_size(prog));
		break;
	}

	if (i == ARRAY_SIZE(payload_ops))
		goto out;

	mirror_payload(payload);

	/* Pass cbtables to payload if architecture desires it. */
	prog_set_entry(&payload->prog, selfload(payload),
			cbmem_find(CBMEM_ID_CBTABLE));

out:
	if (prog_entry(&payload->prog) == NULL)
		die("Payload not loaded.\n");
}

void payload_run(void)
{
	const struct payload *payload = &global_payload;

	/* Reset to booting from this image as late as possible */
	boot_successful();

	printk(BIOS_DEBUG, "Jumping to boot code at %p(%p)\n",
		prog_entry(&payload->prog), prog_entry_arg(&payload->prog));
	post_code(POST_ENTER_ELF_BOOT);

	timestamp_add_now(TS_SELFBOOT_JUMP);

	/* Before we go off to run the payload, see if
	 * we stayed within our bounds.
	 */
	checkstack(_estack, 0);

	arch_payload_run(payload);
}
