[space_glenda_large]: ./icons/spaceglenda300.jpg
[9logo]: ./icons/9logo.jpg
[9power]: ./icons/power36.gif
[screen1]: ./icons/fresh-install.png
[screen2]: ./icons/helloworld.png
[screen3]: ./icons/
[screen4]: ./icons/

# Plan 9: A New Paradigm for Operating Systems
![space_glenda_large]
## ![9logo] What is Plan 9?
Plan 9 is an operating system that sought to reinvent Unix from the ground up, adhering much more firmly to the Unix philosophy by emphasizing minimalism, system modularity, and programming purity. The operating system was developed with programmers, engineers, and researchers in mind, and is specially crafted for distributed computing. As such, it lacks the many amenities that have come to be associated with a modern operating system, such as a pre-installed web browser, custom wallpapers, or a GUI file explorer.

Development began in the late 80s at AT&T's Bell Labs in New Jersey, led by a formidible cast of computer scientists: Rob Pike, Dennis Ritchie, Ken Thompson, and others. Bjarne Stroustrup (creator of C++) and Brian Kernighan (Unix pioneer) would later make their own contributions to the system. The OS grew quickly in popularity within the laboratories, and was first officially released to the public in 1995. Although AT&T would shift focus in the coming years to different operating systems, development on Plan 9 would continue well into the early 2000s, with the final major release occurring in 2002. Updates to the operating system beyond that point mostly consisted of minor bug fixes, license changes, and small system tweaks. The original Plan 9 lineage would come to an end on January 10th, 2015, which marked the operating system's final update. Thanks to the project's open source nature, however, numerous forks of the project have carried on the Plan 9 legacy. These forks include 9front (an attempt at modernization, and the foremost successor), plan9port (ports Plan 9 utilities into modern Unix), and Inferno (a direct descendent, now discontinued), among others.

## ![9logo] Idiosyncracies and Quirks

Plan 9 is exceptionally unique in almost every aspect of it's design, both from a user space and system space perspective. Built primarily for scientists and technical users, it sees the world of computing through a very different lens.

#### The Rio Windowing System

![screen1]
_A fresh install of Plan 9's latest version from January 10, 2015_

Perhaps the steepest part of the learning curve new Plan 9 users will have to surmount is the windowing system, which is drastically different from every other modern windowing system, even including the Unix-based X11. In rio, there are no title bars; middle-mouse clicking on a command -- no matter the source, will execute the command in rc; instead of copy and paste, there's snarf, plumb, and send; windows themselves are just files, as mandated by the Unix philosophy, and the entire system is driven by the 9P file protocol and default Fossil file system. In short, the rio windowing system is pure business -- nothing fancy. It's powerful and flexible, as are most systems in Plan 9, but it's nevertheless a challenge to master.

#### The rc Shell, and a New C Dialect

![screen2]
_Compiling and linking Hello World in the rc shell_

Plan 9 comes pre-packaged with it's own custom shell interface. The main draw of rc, as opposed to other contemporary shells like Bash, zsh, or PowerShell, is its simplicity and minimalism. Comparatively, shell scripts written in rc can be completed in fewer lines, and with fewer special characters than in the Unix-based Bash. For instance, consider the following two one-liners:

__Bash__
```bash
{ a 2>&1 >&3 3>&- | b; } 3>&1
```
__rc__
```rc
a |[2] b
```

Both scripts redirect the stdout of the file 'a' to file descriptor 3, while the stderr of 'a' is redirected into a pipe feeding into file 'b'. Bash's implementation is lengthy, complex, and difficult to read, while rc's version is simple, streamlined, and easily readable.

Not only does Plan 9 boast its own shell, but it also comes complete with it's own set of C compilers, linkers, and standard libraries -- no GNU necessary. The extent to which fundamental aspects of the C programming langauge are reimplemented from the ground up, as opposed to being borrowed, is profound in Plan 9. It is by this nature that Plan 9's also claims it's own emergent strain of the C programming language -- a dialect of sort -- which remains unrivaled in its clarity and purity to this day.

## ![9logo] A Contrasting View on Concurrency and Communication

All modern operating systems, at a bare minimum, should support concurrency of some form. A strong concurrency system is essential to ensuring a safe and performant operating system. The methods used to implement concurrency, however, may differ broadly between different paradigms. There are a variety of ways that Plan 9's concurrency system stands out from the rest. For one, Plan 9 has opted to trade in traditional mutex locks, condition variables, and synchronization primitives for a transparent and impartial system of typed channels, automatically enforced synchronization, and rendezvous-style data communication. Plan 9's reframing of the problem of concurrency enables the operating system to deliver a highly performant and flexible concurrency model that, despite its often baffling level of simplicity, can rival even the most formidible competitors in the field. In fact, despite Plan 9's main branch being inactive since 2015, and having received no major updates since at least 2002, Microsoft elected to adopt Plan 9's custom file server system, 9P, for their Windows Subsystem for Linux toolchain in 2019.

## ![9logo] Citations
* Plan 9 from Bell Labs: https://9p.io/plan9/
* Fansite and archival project: https://cat-v.org/
* The Plan 9 Foundation: https://p9f.org/
* Offical GitHub repository: https://github.com/plan9foundation/plan9

# 

![9power]