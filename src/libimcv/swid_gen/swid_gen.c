/*
 * Copyright (C) 2017 Andreas Steffen
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#define _GNU_SOURCE
#include <stdio.h>

#include "swid_gen.h"

#include <bio/bio_writer.h>

typedef struct private_swid_gen_t private_swid_gen_t;

/**
 * Private data of a swid_gen_t object.
 *
 */
struct private_swid_gen_t {

	/**
	 * Public swid_gen_t interface.
	 */
	swid_gen_t public;

	/**
	 * Path of the SWID generator command
	 */
	char *generator;

	/**
	 * Entity name of the tagCreator
	 */
	char *entity;

	/**
	 * Regid of the tagCreator
	 */
	char *regid;

};

METHOD(swid_gen_t, generate, char*,
	private_swid_gen_t *this, char *sw_id, char *package, char *version,
	bool full, bool pretty)
{
	char *tag = NULL;
	size_t tag_buf_len = 8192;
	char tag_buf[tag_buf_len], command[BUF_LEN];
	bio_writer_t *writer;
	chunk_t tag_chunk;
	FILE *file;

	/* Compose the SWID generator command */
	if (full || !package || !version)
	{ 
		snprintf(command, BUF_LEN, "%s swid --entity-name \"%s\" "
				 "--regid %s --software-id %s%s%s",
				 this->generator, this->entity, this->regid, sw_id,
				 full ? " --full" : "", pretty ? " --pretty" : "");
	}
	else
	{ 
		snprintf(command, BUF_LEN, "%s swid --entity-name \"%s\" "
				 "--regid %s --name %s --version-string %s%s",
				 this->generator, this->entity, this->regid, package,
				 version, pretty ? " --pretty" : "");
	}

	/* Open a pipe stream for reading the SWID generator output */
	file = popen(command, "r");
	if (file)
	{
		writer = bio_writer_create(tag_buf_len);
		while (TRUE)
		{
			if (!fgets(tag_buf, tag_buf_len, file))
			{
				break;
			}
			writer->write_data(writer, chunk_create(tag_buf, strlen(tag_buf)));
		}
		pclose(file);
		tag_chunk = writer->extract_buf(writer);
		writer->destroy(writer);
		if (tag_chunk.len > 1)
		{
			tag = tag_chunk.ptr;
			tag[tag_chunk.len - 1] = '\0';
		}
	}
	else
	{
		DBG1(DBG_IMC, "failed to run swid_generator command");
	}

	return tag;
}

METHOD(swid_gen_t, destroy, void,
	private_swid_gen_t *this)
{
	free(this->generator);
	free(this->entity);
	free(this->regid);
	free(this);
}

/**
 * See header
 */
swid_gen_t *swid_gen_create(char *generator, char* entity, char *regid)
{
	private_swid_gen_t *this;

	INIT(this,
		.public = {
			.generate = _generate,
			.destroy = _destroy,
		},
		.generator = strdup(generator),
		.entity = strdup(entity),
		.regid = strdup(regid),
	);

	return &this->public;
}
