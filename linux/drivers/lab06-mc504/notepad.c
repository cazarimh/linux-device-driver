// SPDX-License-Identifier: GPL-2.0-only
/*  
 *  notepad.c - A simple one-time read notepad module.
 */
#include    <linux/init.h>      /* Needed for __init/__exit */
#include    <linux/module.h>    /* Needed for module_init/module_exit/MODULE_* */
#include    <linux/fs.h>        /* Needed for struct file/struct file_operations */
#include    <linux/uaccess.h>   /* Needed for copy_to_user/copy_from_user */

#define DEVICE_NAME "notepad"
#define BUFFER_SIZE 4097        /* 4097 = 4096 (kernel buffer size) + 1 ('\0') */
#define IOCTL_CLEAR_BUF _IO('N', 1)

static int major;
static int data_len = 0;
static char k_buffer[BUFFER_SIZE] = "";

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  if (cmd == IOCTL_CLEAR_BUF) {
    memset(k_buffer, 0, BUFFER_SIZE);
    data_len = 0;
    pr_info("[%d - INFO] Notepad clear\n", major);
    return 0;
  }
  return -EINVAL;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t max_len, loff_t *offset) {
  pr_info("[%d - DEBUG | R] Found %d bytes to read\n", major, data_len);
  
  size_t read_len = data_len;

  if (!read_len) return 0;

  if (read_len > max_len) read_len = max_len;

  pr_info("[%d - DEBUG | R] Reading %lu bytes\n", major, read_len);

  if (copy_to_user(buf, k_buffer, read_len)) {
    return -EFAULT;
  }

  if (dev_ioctl(file, IOCTL_CLEAR_BUF, 0)) {
    return -EINVAL;
  }

  return read_len;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
  pr_info("[%d - DEBUG | W] Received %lu bytes to write\n", major, len);

  if (data_len >= BUFFER_SIZE - 1) {
    pr_alert("[%d - ALERT] Buffer is full! %lu bytes lost\n", major, len);
    return -ENOSPC;
  }

  size_t write_len = len;

  if (write_len > BUFFER_SIZE - data_len - 1) write_len = BUFFER_SIZE - data_len - 1;

  pr_info("[%d - DEBUG | W] Writing %lu bytes\n", major, write_len);

  if (copy_from_user(k_buffer + data_len, buf, write_len)) {
    return -EFAULT;
  }

  data_len += write_len;
  k_buffer[data_len] = '\0';
  return write_len;
}

static struct file_operations fops = {
  .read = dev_read,
  .write = dev_write,
  .unlocked_ioctl = dev_ioctl,
};

static int __init notepad_init(void) {
  major = register_chrdev(0, DEVICE_NAME, &fops);
  if (major < 0) {
    pr_alert("[ALERT] Error registering the Notepad\n");
    return major;
  }
  pr_info("[%d - INFO] Notepad ON - ID: %d\n", major, major);
  return 0;
}

static void __exit notepad_exit(void) {
  int aux = major;
  unregister_chrdev(major, DEVICE_NAME);
  pr_info("[%d - INFO] Notepad OFF\n", aux);
}

module_init(notepad_init);
module_exit(notepad_exit);

MODULE_AUTHOR("Felipe Verol <f248552@dac.unicamp.br>");
MODULE_AUTHOR("Luiz Lenharo <l237896@dac.unicamp.br>");
MODULE_AUTHOR("Theo Maceres <t220825@dac.unicamp.br>");
MODULE_AUTHOR("Henrique Cazarim <h244763@dac.unicamp.br>");
MODULE_DESCRIPTION("A simple one-time read notepad driver");
MODULE_LICENSE("GPL");