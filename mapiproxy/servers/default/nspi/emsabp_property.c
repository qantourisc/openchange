/*
   OpenChange Server implementation.

   EMSABP: Address Book Provider implementation

   Copyright (C) Julien Kerihuel 2009.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
   \file emsabp_property.c

   \brief Property Tag to AD attributes mapping
 */

#include "dcesrv_exchange_nsp.h"

struct emsabp_property {
	uint32_t	ulPropTag;
	const char	*attribute;
	bool		ref;
	const char	*ref_attr;
};

/* MAPI Property tags to AD attributes mapping */
static const struct emsabp_property emsabp_property[] = {
	{ PidTagAnr,				"anr",			false,	NULL			},
	{ PidTagAccount,			"sAMAccountName",	false,	NULL			},
	{ PR_GIVEN_NAME,			"givenName",		false,	NULL			},
	{ PR_SURNAME,				"sn",			false,	NULL			},
	{ PR_TRANSMITTABLE_DISPLAY_NAME,	"displayName",		false,	NULL			},
	{ PR_7BIT_DISPLAY_NAME,			"displayName",		false,	NULL			},
	{ PR_EMS_AB_HOME_MTA,			"homeMTA",		true,	"legacyExchangeDN"	},
	{ PR_EMS_AB_ASSOC_NT_ACCOUNT,		"assocNTAccount",	false,	NULL			},
	{ PidTagCompanyName,			"company",		false,	NULL			},
	{ PidTagDisplayName,			"displayName",		false,	NULL			},
	{ PidTagDisplayName_string8,		"displayName",		false,	NULL			},
	{ PidTagEmailAddress,			"legacyExchangeDN",	false,	NULL			},
	{ PidTagEmailAddress_string8,		"legacyExchangeDN",	false,	NULL			},
	{ PidTagAddressBookHomeMessageDatabase,	"homeMDB",		true,	"legacyExchangeDN"	},
	{ PidTagAddressBookHomeMessageDatabase_string8,	"homeMDB",		true,	"legacyExchangeDN"	},
	{ PidTagAddressBookProxyAddresses,	"proxyAddresses",	false,	NULL			},
	{ PidTagAddressBookNetworkAddress,	"networkAddress",	false,	NULL			},
	{ PidTagTitle,				"personalTitle",	false,	NULL			},
	{ PR_EMS_AB_OBJECT_GUID,		"objectGUID",		false,	NULL			},
	{ 0,					NULL,			false,	NULL			}
};


/**
   \details Return the AD attribute name associated to a property tag

   \param ulPropTag the property tag to lookup

   \return valid pointer to the attribute name on success, otherwise
   NULL
 */
_PUBLIC_ const char *emsabp_property_get_attribute(uint32_t ulPropTag)
{
	int		i;

	for (i = 0; emsabp_property[i].attribute; i++) {
		if (ulPropTag == emsabp_property[i].ulPropTag) {
			return emsabp_property[i].attribute;
		}
	}
	
	/* if ulPropTag type is PT_UNICODE, turn it to PT_STRING8 */
	if ((ulPropTag & 0xFFFF) == PT_UNICODE) {
		ulPropTag &= 0xFFFF0000;
		ulPropTag += PT_UNICODE;
	} else if ((ulPropTag & 0xFFFF) == PT_MV_STRING8) {
		ulPropTag &= 0xFFFF0000;
		ulPropTag += PT_MV_UNICODE;
	}

	for (i = 0; emsabp_property[i].attribute; i++) {
		if (ulPropTag == emsabp_property[i].ulPropTag) {
			return emsabp_property[i].attribute;
		}
	}

	return NULL;
}


/**
   \details Return the property tag associated to AD attribute name

   \param attribute the AD attribute name to lookup

   \return property tag value on success, otherwise PT_ERROR
 */
_PUBLIC_ uint32_t emsabp_property_get_ulPropTag(const char *attribute)
{
	int		i;

	if (!attribute) return PT_ERROR;

	for (i = 0; emsabp_property[i].attribute; i++) {
		if (!strcmp(attribute, emsabp_property[i].attribute)) {
			return emsabp_property[i].ulPropTag;
		}
	}

	return PT_ERROR;
}


/**
   \details Returns whether the given attribute's value references
   another AD record

   \param ulPropTag the property tag to lookup

   \return 1 if the attribute is a reference, 0 if not and -1 if an
   error occurred.
 */
_PUBLIC_ int emsabp_property_is_ref(uint32_t ulPropTag)
{
	int		i;

	if (!ulPropTag) return -1;

	for (i = 0; emsabp_property[i].attribute; i++) {
		if (ulPropTag == emsabp_property[i].ulPropTag) {
			return (emsabp_property[i].ref == true) ? 1 : 0;
		}
	}

	ulPropTag = (ulPropTag & 0xFFFF) + 0x001e;
	for (i = 0; emsabp_property[i].attribute; i++) {
		if (ulPropTag == emsabp_property[i].ulPropTag) {
			return (emsabp_property[i].ref == true) ? 1 : 0;
		}
	}

	return -1;
}


/**
   \details Returns the reference attr for a given attribute

   \param ulPropTag property tag to lookup

   \return pointer to a valid reference attribute on success,
   otherwise NULL
 */
_PUBLIC_ const char *emsabp_property_get_ref_attr(uint32_t ulPropTag)
{
	int		i;

	if (!ulPropTag) return NULL;

	for (i = 0; emsabp_property[i].attribute; i++) {
		if (ulPropTag == emsabp_property[i].ulPropTag) {
			return emsabp_property[i].ref_attr;
		}
	}

	return NULL;
}
