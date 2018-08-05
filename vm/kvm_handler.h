#ifndef _KVM_HANDLER_H
#define _KVM_HANDLER_H

#include <linux/kvm.h>
#include "vm.h"

int kvm_handle_io(struct vm *vm, struct vcpu *vcpu);
int kvm_handle_hypercall(struct vm *vm, struct vcpu *vcpu);

#endif
