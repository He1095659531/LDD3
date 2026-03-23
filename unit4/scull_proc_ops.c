#include<linux/fs.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include"scull_main.h"
#include"scull_proc_ops.h"

static struct scull_dev *scull_devs;

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
        PDEBUG("==scull_seq_start() enter %p %p %lli\n", s, pos, *pos);
        if ( *pos >= SCULL_DEV_SIZE)
                return NULL;
        else
                return scull_devs + *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
        PDEBUG("==scull_seq_next() enter %p %p %p %lli\n", s , v, pos, *pos);
        (*pos)++;
        return scull_seq_start(s, pos);
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
        PDEBUG("==scull_seq_stop() enter\n");
        return;
}

static int scull_seq_show(struct seq_file *s, void *v)
{
        struct scull_dev *dev = (struct scull_dev *)v;
        struct scull_qset *qs = NULL;
        int j = 0;
        PDEBUG("==scull_seq_show() enter\n");

        seq_printf(s, "\nDevice Scull%d: qset %i, q %i sz %li\n", (int) (dev - scull_devs), dev->qset, dev->quantum, dev->size);
        PDEBUG("\n Device Scull%d: qset %i, q %i sz %li\n", (int) (dev - scull_devs), dev->qset, dev->quantum, dev->size);

        for (qs = dev->data ; qs ; qs = qs->next) {
                seq_printf(s, "item at %p, qset at %p\n", qs, qs->data);
                PDEBUG("item at %p, qset at %p\n", qs, qs->data);
                if (qs->data && !qs->next) {
                        for (; j < dev->qset; j++) {
                                seq_printf(s, "\t%4i: %8p\n", j, qs->data[j]);
                                PDEBUG("\t%4i:%8p\n", j, qs->data[j]);
                        }
                }
        }

        return 0;
}

static struct seq_operations scull_seq_ops = {
    .start = scull_seq_start,
    .stop = scull_seq_stop,
    .next = scull_seq_next,
    .show = scull_seq_show,
};

static int scull_proc_open(struct inode *inode, struct file *filp)
{
    return seq_open(filp, &scull_seq_ops);
}

static const struct proc_ops scull_proc_ops = {
    .proc_open    = scull_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = seq_release,
};

void scull_create_proc(void)
{
    scull_devs = get_scull_devs();
    proc_create("scullseq", 0 ,NULL, &scull_proc_ops);
}

void scull_remove_proc(void)
{
    remove_proc_entry("scullseq", NULL);
}