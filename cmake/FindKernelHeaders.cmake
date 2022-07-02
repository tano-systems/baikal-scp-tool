# Default x86 kernel arch
if(NOT KERNEL_ARCH)
	set(KERNEL_ARCH amd64)
endif()

# Find the kernel release
if(NOT KERNEL_VERSION)
	execute_process(
		COMMAND uname -r
		OUTPUT_VARIABLE KERNEL_RELEASE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else(NOT KERNEL_VERSION)
	set(KERNEL_RELEASE ${KERNEL_VERSION})
endif()

# Find the headers
find_path(KERNELHEADERS_DIR
	include/linux/user.h
	PATHS
		/usr/src/kernels/${KERNEL_RELEASE}
		/usr/src/linux-headers-${KERNEL_RELEASE}-${KERNEL_ARCH}
		/usr/src/linux-headers-${KERNEL_RELEASE}
		/usr/src/linux-headers-${KERNEL_RELEASE}-common
)

# Find the sources
find_path(KERNELSOURCES_DIR
	include/generated/autoconf.h
	PATHS
		/usr/src/kernels/${KERNEL_RELEASE}
		/usr/src/linux-headers-${KERNEL_RELEASE}-${KERNEL_ARCH}
		/usr/src/linux-headers-${KERNEL_RELEASE}
		/usr/src/linux-headers-${KERNEL_RELEASE}-common
)

message(STATUS "Kernel arch: ${KERNEL_ARCH}")
message(STATUS "Kernel release: ${KERNEL_RELEASE}")
message(STATUS "Kernel headers: ${KERNELHEADERS_DIR}")
message(STATUS "Kernel sources: ${KERNELSOURCES_DIR}")
