/***************************************************************************
                          soundsourcemp3.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "soundsourcemp3.h"
#include "trackinfoobject.h"

SoundSourceMp3::SoundSourceMp3( QString sFilename )
{
    QFile file( sFilename.latin1() );
    if (!file.open(IO_ReadOnly))
        qFatal("MAD: Open of %s failed.", sFilename );

    // Read the whole file into inputbuf:
    inputbuf_len = file.size();
    inputbuf = new char[inputbuf_len];
    unsigned int tmp = file.readBlock(inputbuf, inputbuf_len);
    if (tmp != inputbuf_len)
        qFatal("MAD: Error reading mp3-file: %s\nRead only %d bytes, but wanted %d bytes.",sFilename ,tmp,inputbuf_len);

    // Transfer it to the mad stream-buffer:
    mad_stream_init(&Stream);
    mad_stream_buffer(&Stream, (unsigned char *) inputbuf, inputbuf_len);

    /*
      Decode all the headers, and fill in stats:
    */
//    int len = 0;
    mad_header Header;
    filelength = mad_timer_zero;
    bitrate = 0;
    currentframe = 0;
    pos = mad_timer_zero;

    while ((Stream.bufend - Stream.this_frame) > 0)
    {
        if (mad_header_decode (&Header, &Stream) == -1)
        {
            if (!MAD_RECOVERABLE (Stream.error))
                break;
           /* if (Stream.error == MAD_ERROR_LOSTSYNC)
            {
                // ignore LOSTSYNC due to ID3 tags 
                int tagsize = id3_tag_query (Stream.this_frame,
                                Stream.bufend - Stream.this_frame);
                if (tagsize > 0)
                {
                    mad_stream_skip (&Stream, tagsize);
                    continue;
                }
            }*/

            //qDebug("MAD: Error decoding header %d: %s (len=%d)", currentframe, mad_stream_errorstr(&Stream), len);
            continue;
        }
        currentframe++;
        mad_timer_add (&filelength, Header.duration);
        bitrate += Header.bitrate;
        SRATE = Header.samplerate;
    }

    mad_header_finish (&Header);
    if (currentframe==0)
        bitrate = 0;
    else
        bitrate = bitrate/currentframe;
    framecount = currentframe;
    currentframe = 0;

    /*
    qDebug("length  = %ld sec." , filelength.seconds);
    qDebug("frames  = %d" , framecount);
    qDebug("bitrate = %d" , bitrate/1000);
    qDebug("Size    = %d", length());
    */

    // Re-init buffer:
    mad_stream_finish(&Stream);
    mad_stream_init(&Stream);
    mad_stream_buffer(&Stream, (unsigned char *) inputbuf, inputbuf_len);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);

    // Set the type field:
    type = "mp3 file.";

    rest = -1;
}

SoundSourceMp3::~SoundSourceMp3()
{
    mad_stream_finish(&Stream);
    mad_frame_finish(&Frame);
    mad_synth_finish(&Synth);
    delete [] inputbuf;
}

long SoundSourceMp3::seek(long filepos)
{
    int newpos = (int)(inputbuf_len * ((float)filepos/(float)length()));
    //qDebug("Seek to %d %d %d", filepos, inputbuf_len, newpos);

    // Go to an approximate position:
    mad_stream_buffer(&Stream, (unsigned char *) (inputbuf+newpos), inputbuf_len-newpos);
    mad_synth_mute(&Synth);
    mad_frame_mute(&Frame);

    // Decode a few (possible wrong) buffers:
    int no = 0;
    int succesfull = 0;
    while ((no<10) && (succesfull<2)) {
        if (!mad_frame_decode(&Frame, &Stream))
            succesfull ++;
        no ++;
    }

    // Discard the first synth:
    mad_synth_frame(&Synth, &Frame);

    // Remaining samples in buffer are useless
    rest = -1;

    // Unfortunately we don't know the exact fileposition. The returned position is thus an
    // approximation only:
    return filepos;
}

/*    FILE *file;
  Return the length of the file in samples.
*/
inline long unsigned SoundSourceMp3::length()
{
    return (long unsigned) 2*mad_timer_count(filelength, MAD_UNITS_44100_HZ);
}

/*	
  read <size> samples into <destination>, and return the number of
  samples actually read.
*/
unsigned SoundSourceMp3::read(unsigned long samples_wanted, const SAMPLE* _destination)
{
    SAMPLE *destination = (SAMPLE*)_destination;
    unsigned Total_samples_decoded = 0;
    int frames = 0;

    // If samples are left from previous read, then copy them to start of destination
    if (rest > 0)
    {
        for (int i=rest; i<Synth.pcm.length; i++)
        {
            *(destination++) = (SAMPLE)((Synth.pcm.samples[0][i]>>(MAD_F_FRACBITS-14)));

            /* Right channel. If the decoded stream is monophonic then
             * the right output channel is the same as the left one. */
            if (MAD_NCHANNELS(&Frame.header)==2)
                *(destination++) = (SAMPLE)((Synth.pcm.samples[1][i]>>(MAD_F_FRACBITS-14)));
        }
        Total_samples_decoded += 2*(Synth.pcm.length-rest);
    }
    
    //qDebug("Decoding");
    int no;
    while (Total_samples_decoded < samples_wanted)
    {
        if(mad_frame_decode(&Frame,&Stream))
        {
            if(MAD_RECOVERABLE(Stream.error))
            {
                //qDebug("MAD: Recoverable frame level error (%s)",
                //       mad_stream_errorstr(&Stream));
                continue;
            } else if(Stream.error==MAD_ERROR_BUFLEN) {
                //qDebug("MAD: buflen error");
                break;
            } else {
                //qDebug("MAD: Unrecoverable frame level error (%s).",
                //mad_stream_errorstr(&Stream));
                break;
            }
        }
        /* Once decoded the frame is synthesized to PCM samples. No errors
         * are reported by mad_synth_frame();
         */
        mad_synth_frame(&Synth,&Frame);
        frames ++;
        /* Synthesized samples must be converted from mad's fixed
         * point number to the consumer format (16 bit). Integer samples
         * are temporarily stored in a buffer that is flushed when
         * full.
         *
         * the code (MAD_F_FRACBITS-14) used to be (MAD_F_FRACBITS-15). However,
         * this is not enough to avoid out of range when converting to 16 bit
         * (at least on a P3).
         */
        no = min(Synth.pcm.length,(samples_wanted-Total_samples_decoded)/2);
        for (int i=0;i<no;i++)
        {
             /* Left channel */
            *(destination++) = (SAMPLE)((Synth.pcm.samples[0][i]>>(MAD_F_FRACBITS-14)));

            /* Right channel. If the decoded stream is monophonic then
             * the right output channel is the same as the left one. */
            if (MAD_NCHANNELS(&Frame.header)==2)
                *(destination++) = (SAMPLE)((Synth.pcm.samples[1][i]>>(MAD_F_FRACBITS-14)));
        }
        Total_samples_decoded += 2*no;

//        qDebug("decoded: %i, wanted: %i",Total_samples_decoded,samples_wanted);
    }

    // If samples are still left in buffer, set rest to the index of the unused samples
    if (Synth.pcm.length > no)
        rest = no;
    else
        rest = -1;
  
//    qDebug("decoded %i samples in %i frames, rest: %i.", Total_samples_decoded, frames, rest);
    return Total_samples_decoded;
}

void SoundSourceMp3::ParseHeader(TrackInfoObject *Track)
{
    QString location = Track->m_sFilepath+'/'+Track->m_sFilename;

    Track->m_sType = "mp3";

    id3_file *fh = id3_file_open(location.latin1(), ID3_FILE_MODE_READONLY);
    if (fh!=0)
    {
        id3_tag *tag = id3_file_tag(fh);
        if (tag!=0)
        {
            fillField(tag,"TIT2",Track->m_sTitle);
            fillField(tag,"TPE1",Track->m_sArtist);
        }
        id3_file_close(fh);
    }
}

void SoundSourceMp3::getField(id3_tag *tag, const char *frameid, QString str)
{
    id3_frame *frame = id3_tag_findframe(tag, frameid, 0);
    if (frame!=0)
    {
        id3_utf16_t *framestr = id3_ucs4_utf16duplicate(id3_field_getstrings(&frame->fields[1],0));
        int strlen = 0; while (framestr[strlen]!=0) strlen++;
        if (strlen>0)
            str.setUnicodeCodes((ushort *)framestr,strlen);
        delete [] framestr;
    }
}

