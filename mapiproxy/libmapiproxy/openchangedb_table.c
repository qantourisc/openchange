/*
   OpenChange Server implementation

   OpenChangeDB table object implementation

   Copyright (C) Julien Kerihuel 2011

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
   \file openchangedb_table.c

   \brief OpenChange Dispatcher database table routines
 */

#include <inttypes.h>

#include "mapiproxy/dcesrv_mapiproxy.h"
#include "libmapiproxy.h"
#include "libmapi/libmapi.h"
#include "libmapi/libmapi_private.h"

/**
   /details Initialize an openchangedb table

   \param mem_ctx pointer to the memory context to use for allocation
   \param table_type the type of table this object represents
   \param folderID the identifier of the folder this table represents
   \param table_object pointer on pointer to the table object to return

   \return MAPI_E_SUCCESS on success, otherwise MAPISTORE error
 */
_PUBLIC_ enum MAPISTATUS openchangedb_table_init(TALLOC_CTX *mem_ctx, uint8_t table_type, 
						 uint64_t folderID, void **table_object)
{
	struct openchangedb_table	*table;

	/* Sanity checks */
	MAPI_RETVAL_IF(!table_object, MAPI_E_NOT_INITIALIZED, NULL);

	table = talloc_zero(mem_ctx, struct openchangedb_table);
	if (!table) {
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}
	printf("openchangedb_table_init: folderID=%"PRIu64"\n", folderID);
	table->folderID = folderID;
	table->table_type = table_type;
	table->lpSortCriteria = NULL;
	table->restrictions = NULL;
	table->res = NULL;

	*table_object = (void *)table;

	return MAPI_E_SUCCESS;
}


/**
   \details Set sort order to specified openchangedb table object

   \param table_object pointer to the table object
   \param lpSortCriteria pointer to the sort order to save

   \return MAPI_E_SUCCESS on success, otherwise MAPI error
 */
_PUBLIC_ enum MAPISTATUS openchangedb_table_set_sort_order(void *table_object, 
							   struct SSortOrderSet *lpSortCriteria)
{
	struct openchangedb_table	*table;
	int				i;

	/* Sanity checks */
	MAPI_RETVAL_IF(!table_object, MAPI_E_NOT_INITIALIZED, NULL);
	MAPI_RETVAL_IF(!lpSortCriteria, MAPI_E_INVALID_PARAMETER, NULL);

	table = (struct openchangedb_table *) table_object;

	if (table->res) {
		talloc_free(table->res);
	}

	if (table->lpSortCriteria) {
		talloc_free(table->lpSortCriteria);
	}

	table->lpSortCriteria = talloc_zero((TALLOC_CTX *)table, struct SSortOrderSet);
	if (!table->lpSortCriteria) {
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}

	table->lpSortCriteria->cSorts = lpSortCriteria->cSorts;
	table->lpSortCriteria->cCategories = lpSortCriteria->cCategories;
	table->lpSortCriteria->cExpanded = lpSortCriteria->cExpanded;
	table->lpSortCriteria->aSort = talloc_array((TALLOC_CTX *)table->lpSortCriteria, 
						    struct SSortOrder, table->lpSortCriteria->cSorts);
	
	for (i = 0; i < table->lpSortCriteria->cSorts; i++) {
		table->lpSortCriteria->aSort[i].ulPropTag = lpSortCriteria->aSort[i].ulPropTag;
		table->lpSortCriteria->aSort[i].ulOrder = lpSortCriteria->aSort[i].ulOrder;		
	}

	return MAPI_E_SUCCESS;
}


_PUBLIC_ enum MAPISTATUS openchangedb_table_set_restrictions(void *table_object,
							     struct mapi_SRestriction *res)
{
	struct openchangedb_table	*table;

	/* Sanity checks */
	MAPI_RETVAL_IF(!table_object, MAPI_E_NOT_INITIALIZED, NULL);

	table = (struct openchangedb_table *) table_object;

	if (table->res) {
		talloc_free(table->res);
		table->res = NULL;
	}

	if (table->restrictions) {
		talloc_free(table->restrictions);
		table->restrictions = NULL;
	}

	MAPI_RETVAL_IF(!res, MAPI_E_SUCCESS, NULL);

	table->restrictions = talloc_zero((TALLOC_CTX *)table_object, struct mapi_SRestriction);

	switch (res->rt) {
	case RES_PROPERTY:
		table->restrictions->rt = res->rt;
		table->restrictions->res.resProperty.relop = res->res.resProperty.relop;
		table->restrictions->res.resProperty.ulPropTag = res->res.resProperty.ulPropTag;
			table->restrictions->res.resProperty.lpProp.ulPropTag = res->res.resProperty.lpProp.ulPropTag;

			switch (table->restrictions->res.resProperty.lpProp.ulPropTag & 0xFFFF) {
			case PT_STRING8:
				table->restrictions->res.resProperty.lpProp.value.lpszA = talloc_strdup((TALLOC_CTX *)table->restrictions, res->res.resProperty.lpProp.value.lpszA);
				break;
			case PT_UNICODE:
				table->restrictions->res.resProperty.lpProp.value.lpszW = talloc_strdup((TALLOC_CTX *)table->restrictions, res->res.resProperty.lpProp.value.lpszW);
				break;
			default:
				DEBUG(0, ("Unsupported property type for RES_PROPERTY restriction\n"));
				break;
			}
		break;
	default:
		DEBUG(0, ("Unsupported restriction type: 0x%x\n", res->rt));
	}

	return MAPI_E_SUCCESS;
}

static char *openchangedb_table_build_filter(TALLOC_CTX *mem_ctx, struct openchangedb_table *table)
{
	char		*filter = NULL;
	const char	*PidTagAttr = NULL;

	switch (table->table_type) {
	case 0x2 /* EMSMDBP_TABLE_MESSAGE_TYPE */:
		filter = talloc_asprintf(mem_ctx, "(&(PidTagParentFolderId=%"PRIu64")(PidTagMessageId=*)", table->folderID);
		break;
	case 0x1 /* EMSMDBP_TABLE_FOLDER_TYPE */:
		filter = talloc_asprintf(mem_ctx, "(&(PidTagParentFolderId=%"PRIu64")(PidTagFolderId=*)", table->folderID);
		break;
	}

	if (table->restrictions) {
		switch (table->restrictions->rt) {
		case RES_PROPERTY:
			/* Retrieve PidTagName */
			PidTagAttr = openchangedb_property_get_attribute(table->restrictions->res.resProperty.ulPropTag);
			if (!PidTagAttr) {
				talloc_free(filter);
				return NULL;
			}
			filter = talloc_asprintf_append(filter, "(%s=", PidTagAttr);
			switch (table->restrictions->res.resProperty.ulPropTag & 0xFFFF) {
			case PT_STRING8:
				filter = talloc_asprintf_append(filter, "%s)", table->restrictions->res.resProperty.lpProp.value.lpszA);
				break;
			case PT_UNICODE:
				filter = talloc_asprintf_append(filter, "%s)", table->restrictions->res.resProperty.lpProp.value.lpszW);
				break;
			default:
				DEBUG(0, ("Unsupported RES_PROPERTY property type: 0x%.4x\n", (table->restrictions->res.resProperty.ulPropTag & 0xFFFF)));
				talloc_free(filter);
				return NULL;
			}
		}
	}

	/* Close filter */
	filter = talloc_asprintf_append(filter, ")");

	return filter;
}

_PUBLIC_ enum MAPISTATUS openchangedb_table_get_property(TALLOC_CTX *mem_ctx,
							 void *table_object,
							 void *ldb_ctx,
							 char *recipient,
							 uint32_t proptag,
							 uint32_t pos,
							 void **data)
{
	struct openchangedb_table	*table;
	struct ldb_result		*res = NULL;
	char				*ldb_filter = NULL;
	const char * const		attrs[] = { "*", NULL };
	const char			*PidTagAttr = NULL;
	int				ret;

	/* Sanity checks */
	OPENCHANGE_RETVAL_IF(!table_object, MAPI_E_NOT_INITIALIZED, NULL);
	OPENCHANGE_RETVAL_IF(!ldb_ctx, MAPI_E_NOT_INITIALIZED, NULL);
	OPENCHANGE_RETVAL_IF(!recipient, MAPI_E_NOT_INITIALIZED, NULL);
	OPENCHANGE_RETVAL_IF(!data, MAPI_E_NOT_INITIALIZED, NULL);

	table = (struct openchangedb_table *)table_object;

	/* Build ldb filter */
	ldb_filter = openchangedb_table_build_filter(mem_ctx, table);
	OPENCHANGE_RETVAL_IF(!ldb_filter, MAPI_E_TOO_COMPLEX, NULL);
	DEBUG(0, ("ldb_filter = %s\n", ldb_filter));

	/* Fetch results */
	if (table->res) {
		res = table->res;
		printf("cached res->count = %d\n", res->count);
	} else {
		ret = ldb_search(ldb_ctx, (TALLOC_CTX *)table_object, &res,
				 ldb_get_default_basedn(ldb_ctx), LDB_SCOPE_SUBTREE,
				 attrs, ldb_filter, NULL);
		talloc_free(ldb_filter);
		DEBUG(0, ("res->count = %d\n", res->count));
		OPENCHANGE_RETVAL_IF(ret != LDB_SUCCESS || !res->count, MAPI_E_INVALID_PARAMETER, NULL);
		table->res = res;
	}

	/* Ensure position is within search results range */
	OPENCHANGE_RETVAL_IF(pos >= res->count, MAPI_E_INVALID_OBJECT, NULL);

	/* Convert proptag into PidTag attribute */
	if ((table->table_type == 0x2) && proptag == PR_FID) {
		proptag = PR_PARENT_FID;
	}
	PidTagAttr = openchangedb_property_get_attribute(proptag);
	OPENCHANGE_RETVAL_IF(!PidTagAttr, MAPI_E_NOT_FOUND, NULL);

	/* Ensure the element exists */
	OPENCHANGE_RETVAL_IF(!ldb_msg_find_element(res->msgs[pos], PidTagAttr), MAPI_E_NOT_FOUND, NULL);

	/* Check if this is a "special property" */
	*data = openchangedb_get_special_property(mem_ctx, ldb_ctx, recipient, res, proptag, PidTagAttr);
	OPENCHANGE_RETVAL_IF(*data != NULL, MAPI_E_SUCCESS, NULL);

	/* Check if this is NOT a "special property" */
	*data = openchangedb_get_property_data(mem_ctx, res, pos, proptag, PidTagAttr);
	OPENCHANGE_RETVAL_IF(*data != NULL, MAPI_E_SUCCESS, NULL);

	return MAPI_E_NOT_FOUND;
}