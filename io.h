/* MiniDLNA media server
 *
 * This file is part of MiniDLNA.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __IO_H__
#define __IO_H__

#include <stddef.h>

ssize_t io_read_all(int fd, const void *buf, size_t count);
ssize_t io_write_all(int fd, const void *buf, size_t count);

ssize_t io_send_all(int sockfd, const void *buf, size_t len, int flags);

#endif /* __IO_H__ */
