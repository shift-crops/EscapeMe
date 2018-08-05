#ifndef _KVM_HANDLER_H
#define _KVM_HANDLER_H

#include <linux/kvm.h>
#include "vm.h"

void kvm_handle_io(struct vm *vm, struct kvm_regs *regs);
void kvm_handle_hypercall(struct vm *vm, struct kvm_regs *regs);

#endif
