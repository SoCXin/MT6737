

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern int hct_device_dump(struct seq_file *m);

static int info_show(struct seq_file *m, void *v)
{
	//int i;

        hct_device_dump(m);
	return 0;
}

static void *info_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *info_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void info_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations hctinfo_op = {
	.start	= info_start,
	.next	= info_next,
	.stop	= info_stop,
	.show	= info_show
};


static int hctinfo_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &hctinfo_op);
}

static const struct file_operations proc_hctinfo_operations = {
	.open		= hctinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
 static struct proc_dir_entry *hctinfo_config_proc = NULL;

 int  proc_hctinfo_init(void)
{
    printk("%s: entery\n",__func__);  
    hctinfo_config_proc=proc_create("deviceinfo", 0x666, NULL, &proc_hctinfo_operations);
    
    if (hctinfo_config_proc == NULL)
    {
        printk("create_proc_entry hctinfo failed ~~~~\n");
        return -1;
    }
	return 0;
}




