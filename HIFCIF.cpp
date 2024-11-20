// HIFCIF.cpp : This file contains the 'main' function. Program execution begins and ends there.
//OK I HAVE TO DO SOME CODE EDITING ON THIS ONE :))

#include <iostream>

#include <stdafx.h>
#include <globals.h>
using namespace std;


#define	BYTE		unsigned char
#define	WORD16		unsigned short
#define	DWORD		unsigned long

//	BEGIN VD COMPRESSION/DECOMPRESSION GLOBALS
#define N		 	(WORD16)4096				/* size of ring buffer */
#define F		   	(WORD16)18				/* upper limit for match_length */
#define THRESHOLD	(WORD16)2   				/* encode string into position and length
												if match_length is greater than this */
#define NIL			N						/* index for root of binary search trees */

DWORD				textsize = 0,			/* text size counter */
codesize = 0,			/* code size counter */
printcount = 0;			/* counter for reporting progress every 1K bytes */

BYTE				text_buf[N + F - 1];	/* ring buffer of size N, with extra F-1 bytes to
												facilitate string comparison */
short				match_position, match_length,  /* of longest match.  These are set by the
														InsertNode() procedure. */
	lson[N + 1], rson[N + 257], dad[N + 1];  /* left & right children & parents --
												These constitute binary search trees. */
												//	END VD COMPRESSION/DECOMPRESSION GLOBALS

//BYTE        InsertNode , DeleteNode;




												////////////////////////////////////
												//	CONSTRUCTOR
												//
ALGCompress::ALGCompress(void)
{
}


////////////////////////////////////
//	DESTRUCTOR
//
ALGCompress::~ALGCompress(void)
{
}



//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//		COMPRESSION MODULES
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
//	CompressVD()   file->file		OVERLOADED
// Takes in/out filename strings, compresses using the VD
// algorithm, and writes the files.
//
void 	ALGCompress::CompressVD(char* InputFile, char* OutputFile)
{
	short  		i, c, len, r, s, last_match_length, code_buf_ptr;
	BYTE		code_buf[17], mask;
	
	BYTE		EncryptionByte;
	FILE* infile, * outfile;
	char		Buffer[100];



	// open files
	if ((infile = fopen(InputFile, "rb")) == NULL)
	{
		sprintf(Buffer, "can't open %s\n", InputFile);
		//AfxMessageBox(Buffer);
		//AfxAbort();
	}
	if ((outfile = fopen(OutputFile, "wb")) == NULL)
	{
		sprintf(Buffer, "can't open %s\n", OutputFile);
		//AfxMessageBox(Buffer);
		//AfxAbort();
	}


	// clear encryption byte
	EncryptionByte = 0;

	InitTree();  /* initialize trees */

	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;

	for (i = s; i < r; i++)
		text_buf[i] = ' ';  /* Clear the buffer with any character that will appear often. */
	for (len = 0; len < F && (c = getc(infile)) != EOF; len++)
		text_buf[r + len] = (BYTE)c;  /* Read F bytes into the last F bytes of the buffer */

	if ((textsize = len) == 0)
		return;  /* text of size zero */

	for (i = 1; i <= F; i++)
		InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */

	do {
		if (match_length > len)
			match_length = len;  /* match_length may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD)
		{
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		}
		else
		{
			code_buf[code_buf_ptr++] = (unsigned char)match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
					| (match_length - (THRESHOLD + 1)));  /* Send position and
						  length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0)
		{  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
			{
				code_buf[i] += EncryptionByte;
				++EncryptionByte;
				putc(code_buf[i], outfile);     /* code together */
			}
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length && (c = getc(infile)) != EOF; i++)
		{
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = (BYTE)c;	/* read new bytes */
			if (s < F - 1)
				text_buf[s + N] = (BYTE)c;  /* If the position is near the end of buffer,
											//extend the buffer to make string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			/* Since this is a ring buffer, increment the position modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ((textsize += i) > printcount)
		{
			//printf("%12ld\r", textsize);  
			printcount += 1024;
			/* Reports progress each time the textsize exceeds
			   multiples of 1024. */
		}
		while (i++ < last_match_length)
		{	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len)
				InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */

	if (code_buf_ptr > 1)
	{		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++)
		{
			code_buf[i] += EncryptionByte;
			++EncryptionByte;
			putc(code_buf[i], outfile);
		}
		codesize += code_buf_ptr;
	}

	fclose(infile);
	fclose(outfile);

} // end CompressVD


//bozo
//////////////////////////////////////////////////////////
//	CompressVD()   buffer -> file (already opened)	OVERLOADED
//
//	Takes passed char buffer and compresses into file via passed
//	file pointer (caller opens/closes file).
//
void 	ALGCompress::CompressVD(char* ImageBuffer, long BufferLen, FILE* fpOutput)
{
	short  		i, c, len, r, s, last_match_length, code_buf_ptr;
	BYTE		code_buf[17], mask;
	BYTE		EncryptionByte;
	long		CurrentBufferLen;


	// clear buffer offset
	CurrentBufferLen = 0;

	// clear encryption byte
	EncryptionByte = 0;

	InitTree();  /* initialize trees */

	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;

	for (i = s; i < r; i++)
		text_buf[i] = ' ';  /* Clear the buffer with any character that will appear often. */
	for (len = 0; len < F; len++)
	{
		if (CurrentBufferLen >= BufferLen)
			break;
		c = ImageBuffer[CurrentBufferLen++];
		text_buf[r + len] = (BYTE)c;  /* Read F bytes into the last F bytes of the buffer */
	}

	if ((textsize = len) == 0)
		return;  /* text of size zero */

	for (i = 1; i <= F; i++)
		InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */

	do {
		if (match_length > len)
			match_length = len;  /* match_length may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD)
		{
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		}
		else
		{
			code_buf[code_buf_ptr++] = (unsigned char)match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
					| (match_length - (THRESHOLD + 1)));  /* Send position and
						  length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0)
		{  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
			{
				code_buf[i] += EncryptionByte;
				++EncryptionByte;
				putc(code_buf[i], fpOutput);     /* code together */
			}
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length; i++)
		{
			if (CurrentBufferLen >= BufferLen)
				break;
			c = ImageBuffer[CurrentBufferLen++];
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = (BYTE)c;	/* read new bytes */
			if (s < F - 1)
				text_buf[s + N] = (BYTE)c;  /* If the position is near the end of buffer,
											//extend the buffer to make string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			/* Since this is a ring buffer, increment the position modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ((textsize += i) > printcount)
		{
			//printf("%12ld\r", textsize);  
			printcount += 1024;
			/* Reports progress each time the textsize exceeds
			   multiples of 1024. */
		}
		while (i++ < last_match_length)
		{	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len)
				InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */

	if (code_buf_ptr > 1)
	{		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++)
		{
			code_buf[i] += EncryptionByte;
			++EncryptionByte;
			putc(code_buf[i], fpOutput);
		}
		codesize += code_buf_ptr;
	}

} // end CompressVD




//bozo
//////////////////////////////////////////////////////////
//	CompressVD()   buffer -> buffer			OVERLOADED
//
//	Takes passed char buffer and compresses into passed buffer.
//	Returns TRUE on success.
//
short 	ALGCompress::CompressVD(char* ImageBuffer, long BufferLen, char* OutBuffer, long OutBufferLen)
{
	short  		i, c, len, r, s, last_match_length, code_buf_ptr;
	BYTE		code_buf[17], mask;
	BYTE		EncryptionByte;
	long		CurrentBufferLen;
	long		CurrentOutBufferLen;


	// clear buffer offset
	CurrentBufferLen = 0;
	CurrentOutBufferLen = 0;

	// clear encryption byte
	EncryptionByte = 0;

	InitTree();  /* initialize trees */

	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;

	for (i = s; i < r; i++)
		text_buf[i] = ' ';  /* Clear the buffer with any character that will appear often. */
	for (len = 0; len < F; len++)
	{
		if (CurrentBufferLen >= BufferLen)
			break;
		c = ImageBuffer[CurrentBufferLen++];
		text_buf[r + len] = (BYTE)c;  /* Read F bytes into the last F bytes of the buffer */
	}

	if ((textsize = len) == 0)
		return false;  /* text of size zero */

	for (i = 1; i <= F; i++)
		InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */

	do {
		if (match_length > len)
			match_length = len;  /* match_length may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD)
		{
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		}
		else
		{
			code_buf[code_buf_ptr++] = (unsigned char)match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
					| (match_length - (THRESHOLD + 1)));  /* Send position and
						  length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0)
		{  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
			{
				code_buf[i] += EncryptionByte;
				++EncryptionByte;
				// write into output buffer
				if (CurrentOutBufferLen >= OutBufferLen)
				{
					// compression has exceeded the size of the output buffer!
					return false;
				}
				OutBuffer[CurrentOutBufferLen++] = code_buf[i];
				//putc(code_buf[i], fpOutput);     /* code together */
			}
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length; i++)
		{
			if (CurrentBufferLen >= BufferLen)
				break;
			c = ImageBuffer[CurrentBufferLen++];
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = (BYTE)c;	/* read new bytes */
			if (s < F - 1)
				text_buf[s + N] = (BYTE)c;  /* If the position is near the end of buffer,
											//extend the buffer to make string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			/* Since this is a ring buffer, increment the position modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ((textsize += i) > printcount)
		{
			//printf("%12ld\r", textsize);  
			printcount += 1024;
			/* Reports progress each time the textsize exceeds
			   multiples of 1024. */
		}
		while (i++ < last_match_length)
		{	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len)
				InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */

	if (code_buf_ptr > 1)
	{		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++)
		{
			code_buf[i] += EncryptionByte;
			++EncryptionByte;
			// write into output buffer
			if (CurrentOutBufferLen >= OutBufferLen)
			{
				// compression has exceeded the size of the output buffer!
				return false;
			}
			OutBuffer[CurrentOutBufferLen++] = code_buf[i];
			//putc(code_buf[i], fpOutput);
		}
		codesize += code_buf_ptr;
	}

	// default good return
	return true;
} // end CompressVD




//bozo
//////////////////////////////////////////////////////////
//	GetNumCompressedBytes()  
//
//	Takes passed char buffer and its length and runs a TEST compression
//	on it.  Returns the number of required bytes. Returns -1 on error.
//
int 	ALGCompress::GetNumCompressedBytes(char* ImageBuffer, long BufferLen)
{
	short  		i, c, len, r, s, last_match_length, code_buf_ptr;
	BYTE		code_buf[17], mask;
	BYTE		EncryptionByte;
	long		CurrentBufferLen;
	int			NumCompressionBytes;


	// clear number of bytes accumulator
	NumCompressionBytes = 0;

	// clear buffer offset
	CurrentBufferLen = 0;

	// clear encryption byte
	EncryptionByte = 0;

	InitTree();  /* initialize trees */

	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;

	for (i = s; i < r; i++)
		text_buf[i] = ' ';  /* Clear the buffer with any character that will appear often. */
	for (len = 0; len < F; len++)
	{
		if (CurrentBufferLen >= BufferLen)
			break;
		c = ImageBuffer[CurrentBufferLen++];
		text_buf[r + len] = (BYTE)c;  /* Read F bytes into the last F bytes of the buffer */
	}

	if ((textsize = len) == 0)
		return -1;
	//return;  /* text of size zero */

	for (i = 1; i <= F; i++)
		InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */

	do {
		if (match_length > len)
			match_length = len;  /* match_length may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD)
		{
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		}
		else
		{
			code_buf[code_buf_ptr++] = (unsigned char)match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
					| (match_length - (THRESHOLD + 1)));  /* Send position and
						  length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0)
		{  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
			{
				code_buf[i] += EncryptionByte;
				++EncryptionByte;
				++NumCompressionBytes;
				//putc(code_buf[i], fpOutput);     /* code together */
			}
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length; i++)
		{
			if (CurrentBufferLen >= BufferLen)
				break;
			c = ImageBuffer[CurrentBufferLen++];
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = (BYTE)c;	/* read new bytes */
			if (s < F - 1)
				text_buf[s + N] = (BYTE)c;  /* If the position is near the end of buffer,
											//extend the buffer to make string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			/* Since this is a ring buffer, increment the position modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ((textsize += i) > printcount)
		{
			//printf("%12ld\r", textsize);  
			printcount += 1024;
			/* Reports progress each time the textsize exceeds
			   multiples of 1024. */
		}
		while (i++ < last_match_length)
		{	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len)
				InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */

	if (code_buf_ptr > 1)
	{		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++)
		{
			code_buf[i] += EncryptionByte;
			++EncryptionByte;
			++NumCompressionBytes;
			//putc(code_buf[i], fpOutput);
		}
		codesize += code_buf_ptr;
	}

	// return number of bytes required
	return NumCompressionBytes;
} // end GetNumCompressedBytes()
