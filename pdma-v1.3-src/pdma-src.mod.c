#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x77891f6c, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xb5f10442, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x86232093, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xdb7bc396, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x27864d57, __VMLINUX_SYMBOL_STR(memparse) },
	{ 0x15692c87, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x4b94aab2, __VMLINUX_SYMBOL_STR(dev_set_drvdata) },
	{ 0xd8e484f0, __VMLINUX_SYMBOL_STR(register_chrdev_region) },
	{ 0xc8b57c27, __VMLINUX_SYMBOL_STR(autoremove_wake_function) },
	{ 0xad917315, __VMLINUX_SYMBOL_STR(dma_set_mask) },
	{ 0x3171c4e0, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0x8f33dd42, __VMLINUX_SYMBOL_STR(x86_dma_fallback_dev) },
	{ 0xeae3dfd6, __VMLINUX_SYMBOL_STR(__const_udelay) },
	{ 0xa476d16a, __VMLINUX_SYMBOL_STR(pci_release_regions) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xf432dd3d, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x35b6b772, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0xa948a841, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x955c2396, __VMLINUX_SYMBOL_STR(pci_enable_pcie_error_reporting) },
	{ 0xbfaa3c03, __VMLINUX_SYMBOL_STR(pci_iounmap) },
	{ 0x8f64aa4, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0xd343c2df, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x2072ee9b, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x1eda13fa, __VMLINUX_SYMBOL_STR(pci_find_capability) },
	{ 0x552d644d, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x78764f4e, __VMLINUX_SYMBOL_STR(pv_irq_ops) },
	{ 0xd62c833f, __VMLINUX_SYMBOL_STR(schedule_timeout) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0xbc9c3ba2, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0x6aff7790, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x9327f5ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xf76d1cd6, __VMLINUX_SYMBOL_STR(pci_request_regions) },
	{ 0x5c8b5ce8, __VMLINUX_SYMBOL_STR(prepare_to_wait) },
	{ 0x78e42175, __VMLINUX_SYMBOL_STR(pci_disable_msi) },
	{ 0xe846badd, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0xb5dce410, __VMLINUX_SYMBOL_STR(pci_enable_msi_block) },
	{ 0x58b57beb, __VMLINUX_SYMBOL_STR(pci_iomap) },
	{ 0x436c2179, __VMLINUX_SYMBOL_STR(iowrite32) },
	{ 0x166428b, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xc44b90aa, __VMLINUX_SYMBOL_STR(dev_get_drvdata) },
	{ 0x109d5e6, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xe484e35f, __VMLINUX_SYMBOL_STR(ioread32) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v000055AAd00006024sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "65C8337A96E8798656F1CAD");
