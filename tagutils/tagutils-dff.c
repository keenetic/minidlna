//=========================================================================
// FILENAME     : tagutils-dff.c
// DESCRIPTION  : DFF metadata reader
//=========================================================================
// Copyright (c) 2014 Takeshich NAKAMURA
//=========================================================================

/*
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
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define DFF_CKID_FRM8	0x46524d38
#define DFF_CKID_FVER	0x46564552
#define DFF_CKID_PROP	0x50524f50
#define DFF_CKID_COMT	0x434f4d54
#define DFF_CKID_DSD	0x44534420
#define DFF_CKID_DST	0x44535420
#define DFF_CKID_DSTI	0x44535449
#define DFF_CKID_DIIN	0x4449494e
#define DFF_CKID_MANF	0x4d414e46
#define DFF_CKID_FS		0x46532020
#define DFF_CKID_CHNL	0x43484e4c
#define DFF_CKID_CMPR	0x434d5052
#define DFF_CKID_ABSS	0x41425353
#define DFF_CKID_LSCO	0x4c53434f
#define DFF_CKID_FRTE	0x46525445
#define DFF_CKID_DSTF	0x44535446
#define DFF_CKID_DSTC	0x44535443
#define DFF_CKID_EMID	0x454d4944
#define DFF_CKID_MARK	0x4d41524b
#define DFF_CKID_DIAR	0x44494152
#define DFF_CKID_DITI	0x44495449

#define DFF_CKID_SND	0x534E4420

typedef struct {
	uint32_t id;
	uint64_t size;
} __PACKED__ dffChunkHdr;

typedef struct {
	dffChunkHdr hdr;
	uint32_t version;
} __PACKED__ dffFormatVersionChunk;

struct dffFormDSDChunk {
	dffChunkHdr hdr;	// FRM8
	uint32_t type;		// DSD
	dffFormatVersionChunk formatVersionChunk;
	// dffPropertyChunk prop;
	// DSH chunk
} __PACKED__;

struct dffPropertyChunk {
	dffChunkHdr hdr;
	uint32_t propType;	// SND
//	dffSampleRateChunk sampleRateChunk;
//	dffChannelsChunk channelsChunk;
//	dffCompressionTypeChunk compressionTypeChunk;
//	dffAbsoluteStartTimeChunk absoluteStartTimeChunk;
//	dffLoudspeakerConfigurationChunk loudspeakerConfigurationChunk;
} __PACKED__;

struct dffSampleRateChunk {
	dffChunkHdr hdr;
	uint32_t sampleRate;
} __PACKED__;

#define DFF_MAX_CHANNELS 6

struct dffChannelsChunk {
	dffChunkHdr hdr;
	uint16_t numChannels;
	uint32_t channelID[];
} __PACKED__;

struct dffCompressionTypeChunk {
	dffChunkHdr hdr;
	uint32_t compressionType;
	uint8_t count;
	char compressionName[];
} __PACKED__;

struct dffFrameInformationChunk {
	dffChunkHdr hdr;
	uint32_t numFrames;
	uint16_t frameRate;
} __PACKED__;

struct dffArtistChunk {
	dffChunkHdr hdr;
	uint32_t length;
	char name[];
} __PACKED__;

struct dffTitleChunk {
	dffChunkHdr hdr;
	uint32_t length;
	char title[];
} __PACKED__;

static int
_get_dfffileinfo(char *file, struct song_metadata *psong)
{
	FILE *fp;
	uint32_t rt;
	struct dffFormDSDChunk hdr;
	dffChunkHdr ckbuf;
	dffChunkHdr dsdsdckData;

	uint64_t totalsize = 0;
	uint64_t propckDataSize = 0;
	uint64_t count = 0;
	uint32_t samplerate = 0;
	uint16_t channels = 0;
	//DST
	uint64_t dstickDataSize = 0;
	uint32_t numFrames = 0;
	uint16_t frameRate = 0;

	uint64_t totalcount = 0;

	uint32_t compressionType = 0;
	uint64_t dsdsdckDataSize = 0;
	uint64_t cmprckDataSize = 0;
	uint64_t abssckDataSize = 0;
	uint64_t lscockDataSize = 0;
	uint64_t comtckDataSize = 0;
	uint64_t diinckDataSize = 0;
	uint64_t diarckDataSize = 0;
	uint64_t ditickDataSize = 0;
	uint64_t manfckDataSize = 0;

	//DPRINTF(E_DEBUG,L_SCANNER,"Getting DFF fileinfo =%s\n",file);

	if ((fp = fopen(file, "rb")) == NULL)
	{
		DPRINTF(E_WARN, L_SCANNER, "Could not create file handle\n");
		return -1;
	}

	//Form DSD chunk
	if (!(rt = fread(&hdr, sizeof(hdr), 1,fp)))
	{
		DPRINTF(E_WARN, L_SCANNER, "Could not read Form DSD chunk from %s\n", file);
		fclose(fp);
 		return -1;
	}

	if (be32toh(hdr.hdr.id) != DFF_CKID_FRM8)
	{
		DPRINTF(E_WARN, L_SCANNER, "Invalid Form DSD chunk in %s\n", file);
		fclose(fp);
		return -1;
	}

	totalsize = be64toh(hdr.hdr.size);

	if (be32toh(hdr.type) != DFF_CKID_DSD)
	{
		DPRINTF(E_WARN, L_SCANNER, "Invalid Form DSD chunk in %s\n", file);
		fclose(fp);
		return -1;
	}

	//FVER chunk
	if (be32toh(hdr.formatVersionChunk.hdr.id) != DFF_CKID_FVER)
	{
		DPRINTF(E_WARN, L_SCANNER, "Invalid Format Version Chunk in %s\n", file);
		fclose(fp);
		return -1;
	}

	totalsize -= sizeof(dffChunkHdr);
	while (totalcount < totalsize - sizeof(ckbuf.id))
	{
		if (!(rt = fread(&ckbuf, sizeof(ckbuf), 1, fp)))
		{
			//DPRINTF(E_WARN, L_SCANNER, "Could not read chunk header from %s\n", file);
			//fclose(fp);
 			//return -1;
			break;
		}

		const uint32_t id = be32toh(ckbuf.id);
		const uint64_t chunkSize = be64toh(ckbuf.size);

		if (id == DFF_CKID_PROP)
 		{
			//Property chunk
			propckDataSize = chunkSize;
			totalcount += propckDataSize + sizeof(ckbuf);

			unsigned char propckData[propckDataSize];

			if (!(rt = fread(propckData, propckDataSize, 1,fp)))
			{
				DPRINTF(E_WARN, L_SCANNER, "Could not read Property chunk from %s\n", file);
				fclose(fp);
				return -1;
			}

			const uint32_t propType = be32toh(*(const uint32_t*)propckData);

			if (propType != DFF_CKID_SND)
 			{
 				DPRINTF(E_WARN, L_SCANNER, "Invalid Property chunk in %s\n", file);
 				fclose(fp);
 				return -1;
 			}

			count += sizeof(propType);

			while (count < propckDataSize)
			{
				const dffChunkHdr *chunkHdr = (const dffChunkHdr *)(propckData + count);
				const uint32_t chunkId = be32toh(chunkHdr->id);

				if (chunkId == DFF_CKID_FS)
				{
					const struct dffSampleRateChunk *chunk =
						(const struct dffSampleRateChunk *)chunkHdr;

					psong->samplerate = samplerate = be32toh(chunk->sampleRate);
					count += sizeof(struct dffSampleRateChunk);

					//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "Sample Rate is %d\n", psong->samplerate);
				} else if (chunkId == DFF_CKID_CHNL)
				{
					const struct dffChannelsChunk* chunk =
						(const struct dffChannelsChunk *)chunkHdr;
					channels = be16toh(chunk->numChannels);

					if (channels > DFF_MAX_CHANNELS)
					{
						DPRINTF(E_WARN, L_SCANNER, "Invalid channels num %s\n", file);
						fclose(fp);
						return -1;
					}

					psong->channels = channels;
					count += sizeof(dffChunkHdr) + be64toh(chunk->hdr.size);

					//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "channels is %d\n", channels);
				} else if (chunkId == DFF_CKID_CMPR)
				{
					const struct dffCompressionTypeChunk* chunk =
						(const struct dffCompressionTypeChunk *)chunkHdr;
					cmprckDataSize = be64toh(chunk->hdr.size);
					compressionType = be32toh(chunk->compressionType);
					count += sizeof(dffChunkHdr) + cmprckDataSize;

				} else if (chunkId == DFF_CKID_ABSS)
				{
					//Absolute Start Time Chunk
					abssckDataSize = be64toh(chunkHdr->size);
					count += sizeof(dffChunkHdr) + abssckDataSize;

				} else if (chunkId == DFF_CKID_LSCO)
				{
					//Loudsperaker Configuration Chunk
					lscockDataSize = be64toh(chunkHdr->size);
					count += sizeof(dffChunkHdr) + lscockDataSize;

				} else {
					break;
				}
			}

			//bitrate bitpersample is 1bit
			psong->bitrate = channels * samplerate * 1;

			//DSD/DST Sound Data Chunk
			if (!(rt = fread(&dsdsdckData, sizeof(dsdsdckData), 1, fp)))
			{
				DPRINTF(E_WARN, L_SCANNER, "Could not read DSD/DST Sound Data chunk from %s\n", file);
				fclose(fp);
				return -1;
			}

			while (compressionType != be32toh(dsdsdckData.id))
			{
				const uint64_t chunkSize = be64toh(dsdsdckData.size);
				//DPRINTF(E_DEBUG, L_SCANNER, "skip chunk 0x%04x 0x%016llx\n", be32toh(dsdsdckData.id), chunkSize);
				totalcount += chunkSize + sizeof(dsdsdckData);

				if (totalcount >= totalsize - sizeof(uint32_t))
				{
					DPRINTF(E_WARN, L_SCANNER, "eof reached %s\n", file);
					fclose(fp);
					return -1;
				}

				fseeko(fp, chunkSize, SEEK_CUR);

				if (!(rt = fread(&dsdsdckData, sizeof(dsdsdckData), 1,fp)))
				{
					DPRINTF(E_WARN, L_SCANNER, "Could not read DSD/DST Sound Data chunk from %s\n", file);
					fclose(fp);
					return -1;
				}
			}

			const uint32_t dsdsdckDataId = be32toh(dsdsdckData.id);
			dsdsdckDataSize = be64toh(dsdsdckData.size);

			if (dsdsdckDataId == DFF_CKID_DSD)
			{
				totalcount += dsdsdckDataSize + sizeof(dffChunkHdr);
				psong->song_length = (int)((double)dsdsdckDataSize / (double)samplerate / (double)channels * 8 * 1000);

				//DEBUG DPRINTF(E_DEBUG, L_SCANNER, "songlength is %d\n", psong->song_length);

				fseeko(fp, dsdsdckDataSize, SEEK_CUR);

			} else if (dsdsdckDataId == DFF_CKID_DST)
			{
				totalcount += dsdsdckDataSize + sizeof(dffChunkHdr);

				struct dffFrameInformationChunk frameInfoChunk;

				if (!(rt = fread(&frameInfoChunk, sizeof(frameInfoChunk), 1,fp)))
				{
					DPRINTF(E_WARN, L_SCANNER, "Could not read DST Frame Information chunk from %s\n", file);
					fclose(fp);

					return -1;
				}

				if (be32toh(frameInfoChunk.hdr.id) == DFF_CKID_FRTE)
				{
					numFrames = be32toh(frameInfoChunk.numFrames);
					frameRate = be16toh(frameInfoChunk.frameRate);
					psong->song_length = numFrames / frameRate * 1000;

					fseeko(fp, dsdsdckDataSize - sizeof(frameInfoChunk), SEEK_CUR);

				} else
				{
					DPRINTF(E_WARN, L_SCANNER, "Invalid DST Frame Information chunk in %s\n", file);
					fclose(fp);
					return -1;
				}

				dffChunkHdr dstickData;

				//DST Sound Index Chunk
				if (!(rt = fread(&dstickData, sizeof(dstickData), 1,fp)))
				{
					if (ferror(fp))
					{
						DPRINTF(E_WARN, L_SCANNER, "Could not read DST Sound Index chunk from %s\n", file);
						fclose(fp);
						return -1;
					} else
					{
						//EOF
						break;
					}
				}

				if (be32toh(dstickData.id) == DFF_CKID_DSTI)
				{
					dstickDataSize = be64toh(dstickData.size);
					totalcount += dstickDataSize + sizeof(dffChunkHdr);
					fseeko(fp, dstickDataSize, SEEK_CUR);
				} else
				{
					fseeko(fp, -sizeof(dffChunkHdr), SEEK_CUR);
				}
			} else
			{
				DPRINTF(E_WARN, L_SCANNER, "Invalid DSD/DST Sound Data chunk in %s\n", file);
				fclose(fp);
				return -1;
			}
		} else if (id == DFF_CKID_COMT)
		{
			comtckDataSize = chunkSize;
			totalcount += chunkSize + sizeof(dffChunkHdr);

			fseeko(fp, chunkSize, SEEK_CUR);

		} else if (id == DFF_CKID_DIIN)
		{
			//Edited Master Information chunk
			diinckDataSize = chunkSize;
			totalcount += chunkSize + sizeof(ckbuf);

			unsigned char diinckData[chunkSize];

			if (!(rt = fread(diinckData, chunkSize, 1,fp)))
			{
				DPRINTF(E_WARN, L_SCANNER, "Could not read Edited Master Information chunk from %s\n", file);
				fclose(fp);
				return -1;
			}

			uint64_t icount = 0;
			while (icount < chunkSize)
			{
				const dffChunkHdr* diinckHdr = (const dffChunkHdr*)(diinckData+icount);
				const uint32_t diinckID = be32toh(diinckHdr->id);
				const uint32_t diinckSize = be64toh(diinckHdr->size);

				if (diinckID == DFF_CKID_EMID)
				{
					//Edited Master ID chunk
					icount += sizeof(dffChunkHdr) + diinckSize;

				} else if (diinckID == DFF_CKID_MARK)
				{
					//Master chunk
					icount += sizeof(dffChunkHdr) + diinckSize;

				} else if (diinckID == DFF_CKID_DIAR)
				{
					const struct dffArtistChunk* artChunk =
						(const struct dffArtistChunk*)diinckHdr;

					diarckDataSize = diinckSize;
					psong->contributor[ROLE_ARTIST] = strdup(artChunk->name);
					icount += sizeof(dffChunkHdr) + diinckSize;

				} else if (diinckID == DFF_CKID_DITI)
				{
					const struct dffTitleChunk* titleChunk =
						(const struct dffTitleChunk*)diinckHdr;

					ditickDataSize = diinckSize;
					psong->title = strdup(titleChunk->title);
					icount += sizeof(dffChunkHdr) + diinckSize;

				} else
				{
					break;
				}
			}
		} else if (id == DFF_CKID_MANF) // Manufacturer Specific Chunk
		{
			manfckDataSize = chunkSize;
			totalcount += chunkSize + sizeof(ckbuf);

			fseeko(fp, chunkSize, SEEK_CUR);
		}
	}

	fclose(fp);

	//DPRINTF(E_DEBUG, L_SCANNER, "totalsize is 0x%016lx\n", (long unsigned int)totalsize);
	//DPRINTF(E_DEBUG, L_SCANNER, "propckDataSize is 0x%016lx\n", (long unsigned int)propckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "cmprckDataSize is 0x%016lx\n", (long unsigned int)cmprckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "abssckDataSize is 0x%016lx\n", (long unsigned int)abssckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "lscockDataSize is 0x%016lx\n", (long unsigned int)lscockDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "dsdsdckDataSize is 0x%016lx\n", (long unsigned int)dsdsdckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "dstickDataSize is 0x%016lx\n", (long unsigned int)dstickDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "comtckDataSize is 0x%016lx\n", (long unsigned int)comtckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "diinckDataSize is 0x%016lx\n", (long unsigned int)diinckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "diarckDataSize is 0x%016lx\n", (long unsigned int)diarckDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "ditickDataSize is 0x%016lx\n", (long unsigned int)ditickDataSize);
	//DPRINTF(E_DEBUG, L_SCANNER, "manfckDataSize is 0x%016lx\n", (long unsigned int)manfckDataSize);

	//DPRINTF(E_DEBUG, L_SCANNER, "Got dff fileinfo successfully=%s\n", file);
	//DPRINTF(E_DEBUG, L_SCANNER, "TITLE is %s\n",psong->title);
	//DPRINTF(E_DEBUG, L_SCANNER, "ARTIST is %s\n",psong->contributor[ROLE_ARTIST]);
	//DPRINTF(E_DEBUG, L_SCANNER, "samplerate is %d\n", psong->samplerate);
	//DPRINTF(E_DEBUG, L_SCANNER, "song_length is %d\n", psong->song_length);
	//DPRINTF(E_DEBUG, L_SCANNER, "channels are %d\n", psong->channels);
	//DPRINTF(E_DEBUG, L_SCANNER, "bitrate is %d\n", psong->bitrate);

	xasprintf(&(psong->dlna_pn), "DFF");

	return 0;
}
