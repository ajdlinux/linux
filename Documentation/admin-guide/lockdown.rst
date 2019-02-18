Kernel lockdown functionality
-----------------------------

.. CONTENTS
..
.. - Overview.
.. - Enabling Lockdown.

========
Overview
========

Traditionally Linux systems have been run with the presumption that a
process running with full capabilities is effectively equivalent in
privilege to the kernel itself. The lockdown feature attempts to draw
a stronger boundary between privileged processes and the kernel,
increasing the level of trust that can be placed in the kernel even in
the face of hostile processes.

Lockdown can be run in two modes - integrity and confidentiality. In
integrity mode, kernel features that allow arbitrary modification of
the running kernel image are disabled. Confidentiality mode behaves in
the same way as integrity mode, but also blocks features that
potentially allow a hostile userland process to extract secret
information from the kernel.

Note that lockdown depends upon the correct behaviour of the
kernel. Exploitable vulnerabilities in the kernel may still permit
arbitrary modification of the kernel or make it possible to disable
lockdown features.

=================
Enabling Lockdown
=================

Lockdown can be enabled in multiple ways.

Kernel configuration
====================

The kernel can be statically configured by setting either
CONFIG_LOCK_DOWN_KERNEL_FORCE_INTEGRITY or
CONFIG_LOCK_DOWN_KERNEL_FORCE_CONFIDENTIALITY. A kernel configured
with CONFIG_LOCK_DOWN_KERNEL_FORCE_INTEGRITY may be booted into
confidentiality mode using one of the other mechanisms, but otherwise
the kernel will always boot into the configured mode.

Kernel command line
===================

Passing lockdown=integrity or lockdown=confidentiality on the kernel
command line will configure lockdown into the appropriate mode.

Runtime configuration
=====================

/sys/kernel/security/lockdown will indicate the current lockdown
state. The system state may be made stricter by writing either
"integrity" or "confidentiality" into this file, but any attempts to
make it less strict will fail.
