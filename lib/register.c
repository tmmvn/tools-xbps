/*-
 * Copyright (c) 2008-2009 Juan Romero Pardines.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <xbps_api.h>

int SYMEXPORT
xbps_register_pkg(prop_dictionary_t pkgrd, bool automatic)
{
	prop_dictionary_t dict, pkgd;
	prop_array_t array;
	const char *pkgname, *version, *desc, *pkgver;
	char *plist;
	int rv = 0;
	bool autoinst = false;

	plist = xbps_xasprintf("%s/%s/%s", xbps_get_rootdir(),
	    XBPS_META_PATH, XBPS_REGPKGDB);
	if (plist == NULL)
		return EINVAL;

	if (!prop_dictionary_get_cstring_nocopy(pkgrd, "pkgname", &pkgname)) {
		free(plist);
		return EINVAL;
	}
	if (!prop_dictionary_get_cstring_nocopy(pkgrd, "version", &version)) {
		free(plist);
		return EINVAL;
	}
	if (!prop_dictionary_get_cstring_nocopy(pkgrd, "short_desc", &desc)) {
		free(plist);
		return EINVAL;
	}
	if (!prop_dictionary_get_cstring_nocopy(pkgrd, "pkgver", &pkgver)) {
		free(plist);
		return EINVAL;
	}

	if ((dict = prop_dictionary_internalize_from_file(plist)) != NULL) {
		pkgd = xbps_find_pkg_in_dict(dict, "packages", pkgname);
		if (pkgd == NULL) {
			rv = errno;
			goto out;
		}
		if (!prop_dictionary_set_cstring_nocopy(pkgd,
		    "version", version)) {
			prop_object_release(pkgd);
			rv = errno;
			goto out;
		}
		if (!prop_dictionary_set_cstring_nocopy(pkgd,
		    "pkgver", pkgver)) {
			prop_object_release(pkgd);
			rv = errno;
			goto out;
		}
		if (!prop_dictionary_set_cstring_nocopy(pkgd,
		    "short_desc", desc)) {
			prop_object_release(pkgd);
			rv = errno;
			goto out;
		}
		if (!prop_dictionary_get_bool(pkgd,
		    "automatic-install", &autoinst)) {
			if (!prop_dictionary_set_bool(pkgd,
		    	    "automatic-install", automatic)) {
				prop_object_release(pkgd);
				rv = errno;
				goto out;
			}
		}

		/*
		 * Add the requiredby objects for dependent packages.
		 */
		if (pkgrd && xbps_pkg_has_rundeps(pkgrd)) {
			array = prop_dictionary_get(dict, "packages");
			if (array == NULL) {
				prop_object_release(pkgd);
				rv = ENOENT;
				goto out;
			}
			rv = xbps_requiredby_pkg_add(array, pkgrd);
			if (rv != 0) {
				prop_object_release(pkgd);
				goto out;
			}
		}
		/*
		 * Write plist file to storage.
		 */
		if (!prop_dictionary_externalize_to_file(dict, plist))
			rv = errno;
	} else {
		free(plist);
		return ENOENT;
	}
out:
	prop_object_release(dict);
	free(plist);

	return rv;
}

int SYMEXPORT
xbps_unregister_pkg(const char *pkgname)
{
	char *plist;
	int rv = 0;

	assert(pkgname != NULL);

	plist = xbps_xasprintf("%s/%s/%s", xbps_get_rootdir(),
	    XBPS_META_PATH, XBPS_REGPKGDB);
	if (plist == NULL)
		return EINVAL;

	rv = xbps_remove_pkg_dict_from_file(pkgname, plist);
	free(plist);

	return rv;
}
