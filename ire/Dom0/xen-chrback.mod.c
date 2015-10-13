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
	{ 0x45d14bdf, __VMLINUX_SYMBOL_STR(hypercall_page) },
	{ 0x45b1f5db, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x4c4fef19, __VMLINUX_SYMBOL_STR(kernel_stack) },
	{ 0x564d802e, __VMLINUX_SYMBOL_STR(xenbus_dev_error) },
	{ 0x51eafc8e, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xda84218a, __VMLINUX_SYMBOL_STR(xenbus_dev_is_online) },
	{ 0x55526907, __VMLINUX_SYMBOL_STR(xen_features) },
	{ 0x7b3612b9, __VMLINUX_SYMBOL_STR(filp_close) },
	{ 0x8c06a108, __VMLINUX_SYMBOL_STR(xenbus_transaction_start) },
	{ 0x9e3bff2b, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x8b12ea66, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x86623fd7, __VMLINUX_SYMBOL_STR(notify_remote_via_irq) },
	{ 0xf432dd3d, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x15a70c91, __VMLINUX_SYMBOL_STR(free_vm_area) },
	{ 0x8f64aa4, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x429381fe, __VMLINUX_SYMBOL_STR(__xenbus_register_backend) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x4c9d28b0, __VMLINUX_SYMBOL_STR(phys_base) },
	{ 0x1e1f1177, __VMLINUX_SYMBOL_STR(xenbus_unregister_driver) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
	{ 0x3f06aa9f, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xca81ea9a, __VMLINUX_SYMBOL_STR(xenbus_transaction_end) },
	{ 0x26725154, __VMLINUX_SYMBOL_STR(alloc_vm_area) },
	{ 0xd0458ccb, __VMLINUX_SYMBOL_STR(xenbus_strstate) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0x211f68f1, __VMLINUX_SYMBOL_STR(getnstimeofday64) },
	{ 0x2732df78, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x8b04668e, __VMLINUX_SYMBOL_STR(bind_interdomain_evtchn_to_irqhandler) },
	{ 0xb667db7f, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x9327f5ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0xbcd757b2, __VMLINUX_SYMBOL_STR(xenbus_switch_state) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x34f22f94, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x73013896, __VMLINUX_SYMBOL_STR(xenbus_printf) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xde3641ac, __VMLINUX_SYMBOL_STR(xenbus_dev_fatal) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0xf7016530, __VMLINUX_SYMBOL_STR(xenbus_gather) },
	{ 0x47154a3, __VMLINUX_SYMBOL_STR(filp_open) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1A79508D008C4A52D709729");
