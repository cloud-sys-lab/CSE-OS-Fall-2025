[9logo]: ./documentation/icons/9logo.jpg
[9power]: ./documentation/icons/power36.gif

# Folder Structure
## ![9logo] /documentation/
Contains the project's documentation as a Markdown file, along with a folder containing necessary icons.

## ![9logo] /machine/
Contains an Open Virtualization Format file (ovf) that may be imported by a hosted hypervisor, as well as a manifest file (mf), a virtual machine disk (vmdk), and a bootable CD/DVD image (iso). Within the OVF, Plan 9 is fully installed, preconfigured, and logged in as the default user _Glenda_. This machine may be used to explore the OS and run various programs.

## ![9logo] /implementation/
Contains several sample programs that may be executed from the rc shell. Each program demonstrates a key Plan 9 concept, and is written in the Plan 9 C dialect. As such, these programs may only be ran via the Plan 9 compiler and linker. In addition, this folder contains a folder "output", which shows the results of running the implementations in a live Plan 9 virtual machine.

## ![9logo] /presentation/
Contains the PowerPoint presentation demonstrating the OS and some of its key features, in both PPTX and PDF formats.

#

![9power]