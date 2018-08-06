#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "linux/kvm.h"

uint64_t translate(int vcpufd, uint64_t addr){
	struct kvm_translation trans;

	trans.linear_address = addr;
	if(ioctl(vcpufd, KVM_TRANSLATE, &trans)){
		perror("ioctl KVM_TRANSLATE");
		return 0;
	}

	if(!trans.valid)
		return 0;

	return trans.physical_address;
}
