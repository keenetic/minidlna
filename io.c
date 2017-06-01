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

#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "io.h"

ssize_t io_read_all(int fd, const void *buf, size_t count)
{
	uint8_t *p = (uint8_t *)buf;
	size_t size = count;

	while (size > 0)
	{
		const ssize_t ret = read(fd, p, size);

		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return ret;
		}

		p += (uintptr_t)ret;
		size -= (size_t)ret;
	}

	return count;
}

ssize_t io_write_all(int fd, const void *buf, size_t count)
{
	uint8_t *p = (uint8_t *)buf;
	size_t size = count;

	while (size > 0)
	{
		const ssize_t ret = write(fd, p, size);

		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return ret;
		}

		p += (uintptr_t)ret;
		size -= (size_t)ret;
	}

	return count;
}

ssize_t io_send_all(int sockfd, const void *buf, size_t len, int flags)
{
	uint8_t *p = (uint8_t *)buf;
	size_t size = len;

	while (size > 0)
	{
		const ssize_t ret = send(sockfd, p, size, flags);

		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return ret;
		}

		p += (uintptr_t)ret;
		size -= (size_t)ret;
	}

	return len;
}
