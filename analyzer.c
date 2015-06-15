#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1UL << SECTOR_SHIFT)
#define DISK_SIZE (64 * 1024 * 1024)

static int analyzer_major;
static struct gendisk *disk;
static char (*memory)[SECTOR_SIZE];

static int analyzer_open(struct block_device *bdev, fmode_t mode) {
	printk(KERN_ALERT "analyzer_open\n"); // %%%
	return 0;
}

static void analyzer_release(struct gendisk *disk, fmode_t mode) {
	printk(KERN_ALERT "analyzer_release\n"); // %%%
}

static struct block_device_operations analyzer_fops = {
	.owner = THIS_MODULE,
	.open = analyzer_open,
	.release = analyzer_release,
};

static void analyzer_make_request(struct request_queue *q, struct bio *bio) {
	struct bio_vec bvec;
	struct bvec_iter iter;
	printk(KERN_ALERT "analyzer_make_request %c mapping=%p offset=%llx len=%x\n",
		bio_data_dir(bio) == READ ? 'R' : 'W',
		bio_page(bio)->mapping,
		(unsigned long long) bio->bi_iter.bi_sector << SECTOR_SHIFT,
		bio->bi_iter.bi_size); // %%%
	bio_for_each_segment(bvec, bio, iter) {
		char *buffer = __bio_kmap_atomic(bio, iter);
		if (bio_data_dir(bio) == READ) {
			memcpy(buffer, memory[iter.bi_sector], bvec.bv_len);
		} else {
			memcpy(memory[iter.bi_sector], buffer, bvec.bv_len);
		}
		__bio_kunmap_atomic(buffer);
	}
	bio_endio(bio, 0);
}

static int __init analyzer_init(void) {
	analyzer_major = register_blkdev(0, "analyzer");
	if (analyzer_major < 0) {
		return analyzer_major;
	}

	disk = alloc_disk(1);
	if (disk == NULL) {
		unregister_blkdev(analyzer_major, "analyzer");
		return -ENOMEM;
	}
	disk->major = analyzer_major;
	disk->first_minor = 0;
	snprintf(disk->disk_name, DISK_NAME_LEN, "analyzer");
	set_capacity(disk, DISK_SIZE / SECTOR_SIZE);
	disk->fops = &analyzer_fops;

	disk->queue = blk_alloc_queue(GFP_KERNEL);
	if (disk->queue == NULL) {
		put_disk(disk);
		unregister_blkdev(analyzer_major, "analyzer");
		return -ENOMEM;
	}
	blk_queue_make_request(disk->queue, analyzer_make_request);
	blk_queue_logical_block_size(disk->queue, PAGE_CACHE_SIZE);

	memory = vmalloc(DISK_SIZE);
	if (memory == NULL) {
		blk_cleanup_queue(disk->queue);
		put_disk(disk);
		unregister_blkdev(analyzer_major, "analyzer");
		return -ENOMEM;
	}

	add_disk(disk);

	printk(KERN_ALERT "analyzer_init done\n"); // %%%
	return 0;
}

static void __exit analyzer_exit(void) {
	vfree(memory);
	del_gendisk(disk);
	blk_cleanup_queue(disk->queue);
	put_disk(disk);
	unregister_blkdev(analyzer_major, "analyzer");
	printk(KERN_ALERT "analyzer_exit done\n"); // %%%
}

module_init(analyzer_init);
module_exit(analyzer_exit);
