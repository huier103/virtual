#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x7d10d0ab, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x2732df78, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x8b12ea66, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xb667db7f, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x45b1f5db, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xd29aec76, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "29148830E0501AC79E3995D");
