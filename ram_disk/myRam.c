#include <linux/init.h>
#include <linux/module.h>

#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#include <linux/spinlock.h>
#include <linux/vmalloc.h>


#define RAM_DISK_NAME "myRamDisk\0"

static int major_num = 0; // device_num | get when register blk device
static int logical_block_size = 512; // standart kernel blk size
static int nsectors = 1024; /* How big the drive is */
static int minor_num = 1;





static struct blk_device {
	unsigned long size;
	spinlock_t lock;
	uint8_t *data;
	struct request_queue *queue;
	struct gendisk *gd;
};

struct blk_device * myDevice;


// COPIED FUNCTION 
static int kramdisk_getgeo(struct block_device *blk_dev, struct hd_geometry *geo) {
	size_t size;

	printk(KERN_NOTICE "%s called\n", __FUNCTION__);
	size = myDevice->size;
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	
	return 0;
}

//example
/*static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
        unsigned long nsect, char *buffer, int write)
{
    unsigned long offset = sector*KERNEL_sectOR_SIZE;
    unsigned long nbytes = nsect*KERNEL_sectOR_SIZE;

    if ((offset + nbytes) > dev->size) {
        printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }
    if (write)
        memcpy(dev->data + offset, buffer, nbytes);
    else
        memcpy(buffer, dev->data + offset, nbytes);
}
*/
static void _ramdisk_request_worker(struct blk_device *dev, unsigned long sector,
	unsigned long nsect,char *buff,  int write) 
{
	unsigned long offset = sector*logical_block_size;
	unsigned long nbytes = nsect*logical_block_size;
	
	if ((offset + nbytes) > dev->size) {
		printk(KERN_ALERT "TOO LARGE BUFF\n");
		return;
	}
	
	if (write) {
		memcpy(dev->data, buff, nbytes);
	} else {
		memcpy(buff, dev->data + offset, nbytes);
	}
}




/* example
static void sbull_request(request_queue_t *q)
{
    struct request *req;

    while ((req = elv_next_request(q)) != NULL) {
        struct sbull_dev *dev = req->rq_disk->private_data;
        if (! blk_fs_request(req)) {
            printk (KERN_NOTICE "Skip non-fs request\n");
            end_request(req, 0);
            continue;
        }
        sbull_transfer(dev, req->sector, req->current_nr_sectors,
                req->buffer, rq_data_dir(req));
        end_request(req, 1);
    }
} */
static void ramdisk_request(struct request_queue *queue)
{
	struct request *req;
	
	while ((req = blk_fetch_request(queue)) != NULL) {
		struct blk_device *dev = req->rq_disk->private_data;
		if (req->cmd_type != REQ_TYPE_FS) {
			printk(KERN_NOTICE "Request has not fs type\n");
			blk_end_request_cur(req, -EINVAL);
			continue;
		}
		_ramdisk_request_worker(dev, blk_rq_pos(req), blk_rq_cur_sectors(req),
			 req->buffer, rq_data_dir(req)); // process the request
				 
				 
		blk_end_request_cur(req, 0);// delete last request
	}
}

static struct block_device_operations fops = {
	.owner = THIS_MODULE,
	.getgeo = kramdisk_getgeo
};

static int ram_init(void) 
{
	int ret = 0;
	major_num = register_blkdev(major_num, RAM_DISK_NAME);
	if (major_num <= 0) {
		printk(KERN_ALERT "Can't allocate major number\n");
		return -EBUSY;
	}
	printk(KERN_ALERT "got major_num\n");
	myDevice = kmalloc(sizeof(struct blk_device), GFP_KERNEL);
	if (myDevice == NULL) {
		printk(KERN_ALERT "Can't allocate kmem for device\n");
		ret = -ENOMEM;
		goto out_unregister;
	}
	printk(KERN_ALERT "allocated memory\n");
	
	// set 0 into struct
	memset(myDevice, 0, sizeof(struct blk_device));
	
	// SET blk_device memory
	myDevice->size = nsectors * logical_block_size;
	myDevice->data = vmalloc(myDevice->size);
	if (myDevice->data == NULL) {
		printk(KERN_ALERT "Can't allocate memory for device's data\n");
		ret = -ENOMEM;
		goto out_noDataMem;
	}
	printk(KERN_ALERT "Allocated device's memory\n");
	
		// INIT SPIN_LOCK FOR QUEUE
	spin_lock_init(&myDevice->lock);
	
	
	myDevice->queue = blk_init_queue(ramdisk_request, &myDevice->lock);
	if (myDevice->queue == NULL) {
		printk(KERN_ALERT "Can't allcoate memory for queue\n");
		ret = -ENOMEM;
		goto out_noQueueMem;
	}
	
	// some magic
	blk_queue_physical_block_size(myDevice->queue, logical_block_size);
	blk_queue_logical_block_size(myDevice->queue, logical_block_size);
	
	myDevice->gd = alloc_disk(minor_num);
	if (myDevice->gd == NULL) {
		printk(KERN_ALERT "Can't allocate disk\n");
		ret = -ENOMEM;
		goto out_noGdMem;
	}
	myDevice->gd->major = major_num;
	myDevice->gd->first_minor = 1; 
	myDevice->gd->fops = &fops;
	myDevice->gd->queue = myDevice->queue;
	myDevice->gd->private_data = myDevice;
	snprintf(myDevice->gd->disk_name, 11, "%s%c", RAM_DISK_NAME, 0); 
	set_capacity(myDevice->gd, nsectors ); 
	
	
	add_disk(myDevice->gd); //last func
	printk(KERN_ALERT "End of  blk_device INIT\n");

	
	
out_noGdMem:
	blk_cleanup_queue(myDevice->queue);

out_noQueueMem:
	vfree(myDevice->data);
	
out_noDataMem:
	kfree(myDevice);
	
out_unregister:
	unregister_blkdev(major_num, RAM_DISK_NAME);
	
	
	
	return ret;
}


static void ram_exit(void) {
	del_gendisk(myDevice->gd);
	put_disk(myDevice->gd);
	unregister_blkdev(major_num, RAM_DISK_NAME);
	blk_cleanup_queue(myDevice->queue);
	vfree(myDevice->data);
	printk(KERN_ALERT "Freed blk_device's memory\n");
}

module_init(ram_init);
module_exit(ram_exit);


