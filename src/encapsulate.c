#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vorbis/vorbisenc.h>
#include <algorithm>
using namespace std;

#define READ 2048
signed char readbuffer[READ]; /* out of the data segment, not the stack */

int main(){
  ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  int eos=0,ret;
  int i, founddata;

  ogg_stream_init(&os,rand());
//   ogg_stream_reset(&os);
//   ogg_stream_init(&os,rand());
//   os.b_o_s = 1;
  os.e_o_s = 0;
  op.packet =(unsigned char*)readbuffer;
  op.bytes = READ;
  op.b_o_s = 2;
  op.e_o_s = 0;
  op.granulepos=-1;
  op.packetno=-1;
  while(!eos){
    long i;
    long bytes=fread(readbuffer, 1, READ, stdin);
    eos =0;
    if(bytes==0) { // EOF
      eos = 1;
    }
    else {
      op.b_o_s = max(--(op.b_o_s), (long int)0);
      op.e_o_s = 0;
      op.granulepos++;
      op.packetno++;
      /* weld the packet into the bitstream */
      ogg_stream_packetin(&os,&op);
      /* write out pages (if any) */
      while(!eos){
        int result=ogg_stream_pageout(&os,&og);
        if(result==0)break;
        fwrite(og.header,1,og.header_len,stdout);
        fwrite(og.body,1,og.body_len,stdout);
	    }
    }
  }

  /* clean up and exit.  vorbis_info_clear() must be called last */

  ogg_stream_clear(&os);

  /* ogg_page and ogg_packet structs always point to storage in
     libvorbis.  They're never freed or manipulated directly */

  fprintf(stderr,"Done.\n");
  return(0);
}
