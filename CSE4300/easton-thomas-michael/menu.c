#include <synch.h>
#include <types.h>
#include <vnode.h>
#include <uio.h>
#include <vfs.h>
#include <sfs.h>
#include <machine/spl.h>
#include <array.h>
static struct semaphore *done_sem;


/*
 * Helper function for cmd_echo to handle escape characters
 */
void print_with_escapes(const char *str) {
	int i;
	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == '\\') {
			i++;
			switch (str[i]) {
                case 'a': kprintf("%c", '\a'); break;
                case 'b': kprintf("%c", '\b'); break;
                case 'r': kprintf("%c", '\r'); break;
                case 'v': kprintf("%c", '\v'); break;
				case 'n': kprintf("%c", '\n'); break;
				case 't': kprintf("%c", '\t'); break;
				case '\\': kprintf("%c", '\\'); break;
				default: kprintf("%c", '\\'); kprintf("%c", str[i]); break;
			}
		} else {
			kprintf("%c", str[i]);
		}
	}
}

/*
 * Command for echoing a string
 * By Easton
 */
static int cmd_echo(int nargs, char **args) {
	if (nargs <= 1) {
		kprintf("Usage: echo [option] String\n");
		return EINVAL;
	}

	int i = 1;
	char *option = args[1];
	
	if (strcmp(option, "--help") == 0) {
		kprintf("Usage: echo [option] String\nOptions:\n\t-n: Removes new line after echo\n\t-E: Default option\n\t-e: Enables escape characters (i.e. \\n, \\t, etc.)\n\t--help: Shows this menu");
		return 0;
	}

	if (!strcmp(option, "-n") || !strcmp(option, "-E") || !strcmp(option, "-e")) i++;
	
	for (i; i < nargs; i++) {
		if (strcmp(option, "-e")) {
			kprintf("%s", args[i]);
		} else {
			
			print_with_escapes(args[i]);
		}

		if (i < nargs - 1) {
			kprintf(" ");
		}
	}
	if (strcmp(option, "-n")) kprintf("\n");
	return 0;
}







/*
 * Put the actual child thread asleep for n seconds
 * By Thomas
 */
static
int
sleep_thread(void* ptr, unsigned long n)
{
        kprintf("Child sleeping for %lu seconds\n", n);
        clocksleep(n);
        kprintf("Child done sleeping\n");
        V(done_sem);
        return 0;
}

/*
 * Command blocking shell for n sec.
 */
static
int
cmd_sleep(int nargs, char **args)
{
        int result;
        unsigned long n;

        // Semaphore 
        done_sem = sem_create("done_sem", 0);
        if (done_sem == NULL) {
                kprintf("Could not create semaphore\n");
                return 1;
        }

        // Taking number
        if (nargs < 2) {
                kprintf("Usage: sleep <seconds>\n");
                return 1;
        }
        n = (unsigned long)atoi(args[1]);
        if (result) {
                kprintf("Invalid number\n");
                return 1;
        }

        // Create child
        result = thread_fork(args[0], NULL, n, sleep_thread, NULL);

        // Wait for the child to finish
        kprintf("Parent waiting for child to finish\n");
        P(done_sem);
        kprintf("Parent continuing\n");

        return 0;
}



static
int
cmd_cat(int nargs, char **args)
{
    int i, result;
    char *filename;
    struct vnode *v;    
    struct uio ku;        // kernel-space UIO structure
    char buffer[512];     // buffer to read data into
    off_t offset;         // position in the file

    // require at least one file
    if (nargs <= 1) {
        kprintf("Usage: cat <filename1> [filename2] ...\n");
        return EINVAL;
    }

    for (i = 1; i < nargs; i++) {
        filename = args[i];
        offset = 0; 
        result = vfs_open(filename, O_RDONLY, &v);
        
        if (result) {
            kprintf("cat: %s: %s\n", filename, strerror(result));
            continue;  
        }

        while (1) {
            mk_kuio(&ku, buffer, sizeof(buffer) - 1, offset, UIO_READ);

            result = VOP_READ(v, &ku);
            if (result) {
                kprintf("cat: Error reading %s: %s\n", filename, strerror(result));
                break;
            }

            size_t bytes_read = (sizeof(buffer) - 1) - ku.uio_resid;

            if (bytes_read == 0) {
                break;
            }

            buffer[bytes_read] = '\0';
            
            kprintf("%s", buffer);

            offset = ku.uio_offset;
        }

        vfs_close(v);
    }

    return 0;
}



static int cmd_ps(int nargs, char **args) {
	if (nargs != 1) {
		kprintf("Usage: ps\n");
		return EINVAL;
	}

	int s = splhigh();
    extern struct array *allthreads;

	kprintf("TID\tNAME\n");
	int i;
	for (i = 0; i < array_getnum(allthreads); i++) {
		struct thread *t = array_getguy(allthreads, i);
		kprintf("%3u\t%s\n", t->thread_id, t->t_name);
	}

	splx(s);
	
	return 0;
}

/*
 * ls command by Thomas 
 * Works only with no arguments. Arguments are ignored.
 */
int cmd_ls(int nargs, char **args) {

        int err;
        struct vnode *v;
        struct uio ku;
        struct sfs_dir d;
        off_t offset = 0;

        err = vfs_open(".", O_RDONLY, &v);
        if (err) {
                kprintf("Error while opening file\n");
                return 1;
        }
        while (1) {
                struct iovec iov;
                char buffer[sizeof(struct sfs_dir)];
                iov.iov_un.un_kbase = &d;
                iov.iov_len = sizeof(d);
                ku.uio_iovec = iov;
                ku.uio_offset = offset;
                ku.uio_resid = sizeof(buffer);
                ku.uio_segflg = UIO_SYSSPACE;
                ku.uio_rw = UIO_READ;
                ku.uio_space = NULL;

                err = VOP_GETDIRENTRY(v, &ku);
                if (err) break;
                if (ku.uio_resid == sizeof(d)) break;
                offset = ku.uio_offset;

                kprintf("%s\n", d.sfd_name);
        }
        vfs_close(v);
        return 0;
}




static struct {
	const char *name;
	int (*func)(int nargs, char **args);
} cmdtable[] = {
    /* operations */
    { "echo", 	cmd_echo },
    { "sleep",  cmd_sleep },
    { "cat",    cmd_cat },
    { "ps",     cmd_ps },
    { "ls",     cmd_ls },

}
