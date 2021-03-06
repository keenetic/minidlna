//=========================================================================
// FILENAME	: tagutils-lavf.c
// DESCRIPTION	: LAVF metadata reader
//=========================================================================
// Copyright (c) 2012, AVP. All Rights Reserved.
//=========================================================================

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "misc.h"

#define NDM_CONV_ICONV_COMPAT
#include <ndm/conv.h>

static void meta_parse(struct song_metadata *psong, const char *key, const char *val) {
	if( !strcasecmp(key, "ALBUM") ) {
		if( *val ) psong->album = strdup(val);
	} else if( !strcasecmp(key, "ARTISTSORT") ) {
		if( *val ) psong->contributor_sort[ROLE_ARTIST] = strdup(val);
	} else if( !strcasecmp(key, "ARTIST") ) {
		if( *val ) psong->contributor[ROLE_ARTIST] = strdup(val);
	} else if( !strcasecmp(key, "TITLE") ) {
		if( *val ) psong->title = strdup(val);
	} else if( !strcasecmp(key, "TRACKNUMBER") ) {
		psong->track = atoi(val);
	} else if( !strcasecmp(key, "DISCNUMBER") ) {
		psong->disc = atoi(val);
	} else if( !strcasecmp(key, "GENRE") )	{
		if( *val ) psong->genre = strdup(val);
	} else if( !strcasecmp(key, "DATE") ) {
		if( strlen(val) >= 10 &&
		   isdigit(val[0]) && isdigit(val[1]) && ispunct(val[2]) &&
		   isdigit(val[3]) && isdigit(val[4]) && ispunct(val[5]) &&
		   isdigit(val[6]) && isdigit(val[7]) && isdigit(val[8]) && isdigit(val[9])) 		{
			// nn-nn-yyyy
			psong->year = atoi(val + 6);
		} else {
			// year first. year is at most 4 digit.
			psong->year = atoi(val);
		}
	} else if( !strcasecmp(key, "COMMENT") ) {
		if( *val ) psong->comment = strdup(val);
	} else if( !strcasecmp(key, "MUSICBRAINZ_ALBUMID") ) {
		if( *val ) psong->musicbrainz_albumid = strdup(val);
	} else if( !strcasecmp(key, "MUSICBRAINZ_TRACKID") ) {
		if( *val ) psong->musicbrainz_trackid = strdup(val);
	} else if( !strcasecmp(key, "MUSICBRAINZ_TRACKID") ) {
		if( *val ) psong->musicbrainz_trackid = strdup(val);
	} else if( !strcasecmp(key, "MUSICBRAINZ_ARTISTID") ) {
		if( *val ) psong->musicbrainz_artistid = strdup(val);
	} else if( !strcasecmp(key, "MUSICBRAINZ_ALBUMARTISTID") ) {
		if( *val ) psong->musicbrainz_albumartistid = strdup(val);
	}
}

static int _get_flctags(char *file, struct song_metadata *psong) {
	psong->lossless = 1;
	psong->vbr_scale = 1;
	return 0;
}

static int _get_wavtags(char *file, struct song_metadata *psong) {
	psong->lossless = 1;
	psong->vbr_scale = 1;
	return 0;
}

static int _get_oggtags(char *file, struct song_metadata *psong) {
	psong->lossless = 0;
	psong->vbr_scale = 0;
	return 0;
}

static int _get_mp3tags(char *file, struct song_metadata *psong) {
	psong->lossless = 0;
	psong->vbr_scale = 0;
	return 0;
}

static int _get_lavffileinfo(char *filename, struct song_metadata *psong) {
	AVFormatContext *ic;
	AVMetadataTag *tag;
	AVMetadata *m;
	int err;
	iconv_t cnv;
	char cutf8[1024];
	size_t inbytes, outbytes;
	const char *in;
	char *out;
	
	av_register_all();
	if( (err = av_open_input_file(&ic, filename, NULL, 0, NULL)) < 0 ) 
		return err;
  ic->max_analyze_duration = 1;
	if( (err = av_find_stream_info(ic)) < 0 ) { 
		av_close_input_file(ic);
		return err;
	}
	
	cnv = iconv_open("utf-8", "cp1251");
	tag = NULL;
	m = ic->metadata;
	if( m ) {
		while( (tag = av_metadata_get(m, "", tag, AV_METADATA_IGNORE_SUFFIX)) ) {
			if( cnv != (iconv_t)-1 && !is_utf8(tag->value) ) {
				in = tag->value;
				out = cutf8;
				inbytes = strlen(tag->value) + 1;
				outbytes = sizeof(cutf8);
				err = iconv(cnv, &in, &inbytes, &out, &outbytes);
				meta_parse(psong, tag->key, cutf8);
			} else meta_parse(psong, tag->key, tag->value);
		}
	}
	
	if( ic->duration <= 0 ) psong->song_length = 0; 
	else psong->song_length = ((ic->duration * 1000) / AV_TIME_BASE);
			
	if( ic->nb_streams ) {
		AVCodecContext *dec = ic->streams[0]->codec;
		
		if( dec->codec_type == AVMEDIA_TYPE_AUDIO ) {
			psong->samplerate = dec->sample_rate;
			psong->channels = dec->channels;
			psong->bitrate = dec->bit_rate;
		}
	}
	
	if( !psong->bitrate && psong->song_length >= 8 ) 
		psong->bitrate = (((uint64_t)(psong->file_size) * 1000) / (psong->song_length / 8));
	
	av_close_input_file(ic);	
	if( cnv != (iconv_t)-1 ) iconv_close(cnv);
	return 0;
}
