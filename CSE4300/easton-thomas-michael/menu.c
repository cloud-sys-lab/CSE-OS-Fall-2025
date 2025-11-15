#include <synch.h>
#include <types.h>
#include <vnode.h>
#include <types.h>
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
int
cmd_ls(int nargs, char **args)
{
    int err;
    struct vnode *v;
    char *path;
    off_t offset = 0;

    if (nargs > 1) {
        path = args[1];
    } else {
        path = ".";
    }

    err = vfs_open(path, O_RDONLY, &v);
    if (err) {
        kprintf("ls: failed to open file/directory %s (err=%d)\n", path, err);
        return 1;
    }

    while (1) {
        struct uio ku;
        char name[NAME_MAX + 1];
        // initialize name
        bzero(name, sizeof(name));

        // define uio
        mk_kuio(&ku, name, sizeof(name), offset, UIO_READ);
        ku.uio_segflg = UIO_SYSSPACE;
        ku.uio_space = NULL;

        // read entry
        err = VOP_GETDIRENTRY(v, &ku);

        // Check errors or end of file
        if (err) {
            break;
        }
        
        if (ku.uio_resid == sizeof(name)) {
            break;
        }
        
        // update offset
        offset= ku.uio_offset;
        

        kprintf("%s\n", name);
    }

    vfs_close(v);
    return 0;
}



static
int
cmd_cp(int nargs, char **args)
{
    char *source_file, *dest_file;
    struct vnode *v_source, *v_dest;
    struct uio ku_read, ku_write;
    char buffer[512]; // 512 byte buffer
    off_t offset;
    int result, n_read;

    if (nargs != 3) {
        kprintf("cp <source_file> <destination_file>\n");
        return EINVAL;
    }

    source_file = args[1];
    dest_file = args[2];
    offset = 0; // start at beginning of file

    result = vfs_open(source_file, O_RDONLY, &v_source);
    if (result) {
        kprintf("cp: %s: %s\n", source_file, strerror(result));
        return result;
    }

    result = vfs_open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, &v_dest);
    if (result) {
        kprintf("cp: %s: %s\n", dest_file, strerror(result));
        vfs_close(v_source); // clean up source file
        return result;
    }

    while (1) {
        mk_kuio(&ku_read, buffer, sizeof(buffer), offset, UIO_READ);

        // read from the source
        result = VOP_READ(v_source, &ku_read);
        if (result) {
            kprintf("cp: Error reading %s: %s\n", source_file, strerror(result));
            break;
        }

        // check how much was read
        n_read = sizeof(buffer) - ku_read.uio_resid;

        if (n_read == 0) {
            // end of the file
            break;
        }

        mk_kuio(&ku_write, buffer, n_read, offset, UIO_WRITE);

        // write to destination
        result = VOP_WRITE(v_dest, &ku_write);
        if (result) {
            kprintf("cp: Error writing %s: %s\n", dest_file, strerror(result));
            break;
        }

        // update offset for next loop
        offset += n_read;
    }

    // close files
    vfs_close(v_source);
    vfs_close(v_dest);

    // result should return 0 if works and no problems
    return result;
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
	{ "cp",		cmd_cp }

}



