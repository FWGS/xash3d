/*
sv_custom.c - downloading custom resources
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "server.h"

#define RES_CHECK_CONSISTENCY		(1<<7)	// check consistency for this resource

//=======================================================================
//
//			UNDER CONSTRUCTION
//
//=======================================================================

typedef struct
{
	char	*filename;
	int	num_items;
	file_t	*file;	// pointer to wadfile
} cachewad_t;

int CustomDecal_Init( cachewad_t *wad, byte *data, int size, int playernum )
{
	// TODO: implement
	return 0;
}

void *CustomDecal_Validate( byte *data, int size )
{
	// TODO: implement
	return NULL;
}

int SV_CreateCustomization( customization_t *pListHead, resource_t *pResource, int playernumber, int flags, customization_t **pCust, int *nLumps )
{
	customization_t	*pRes;
	cachewad_t	*pldecal;
	qboolean		found_problem;

	found_problem = false;

	ASSERT( pResource != NULL );

	if( pCust != NULL ) *pCust = NULL;

	pRes = Z_Malloc( sizeof( customization_t ));
	pRes->resource = *pResource;

	if( pResource->nDownloadSize <= 0 )
	{
		found_problem = true;
	}
	else
	{
		pRes->bInUse = true;

		if(( flags & FCUST_FROMHPAK ) && !HPAK_GetDataPointer( "custom.hpk", pResource, (byte **)&(pRes->pBuffer), NULL ))
		{
			found_problem = true;
		}
		else
		{
			pRes->pBuffer = FS_LoadFile( pResource->szFileName, NULL, false );

			if(!( pRes->resource.ucFlags & RES_CUSTOM ) || pRes->resource.type != t_decal )
			{
				// break, except we're not in a loop
			}
			else
			{
				pRes->resource.playernum = playernumber;

				if( !CustomDecal_Validate( pRes->pBuffer, pResource->nDownloadSize ))
				{
					found_problem = true;
				}
				else if( flags & RES_CUSTOM )
				{
				}
				else
				{
					pRes->pInfo = Z_Malloc( sizeof( cachewad_t ));
					pldecal = pRes->pInfo;

					if( pResource->nDownloadSize < 1024 || pResource->nDownloadSize > 16384 )
					{
						found_problem = true;
					}
					else if( !CustomDecal_Init( pldecal, pRes->pBuffer, pResource->nDownloadSize, playernumber ))
					{
						found_problem = true;
					}
					else if( pldecal->num_items <= 0 )
					{
						found_problem = true;
					}
					else
					{
						if( nLumps ) *nLumps = pldecal->num_items;

						pRes->bTranslated = 1;
						pRes->nUserData1 = 0;
						pRes->nUserData2 = pldecal->num_items;

						if( flags & FCUST_WIPEDATA )
						{
							Mem_Free( pldecal->filename );
							FS_Close( pldecal->file );
							Mem_Free( pldecal );
							pRes->pInfo = NULL;
						}
					}
				}
			}
		}
	}

	if( !found_problem )
	{
		if( pCust ) *pCust = pRes;

		pRes->pNext = pListHead->pNext;
		pListHead->pNext = pRes;
	}
	else
	{
		if( pRes->pBuffer ) Mem_Free( pRes->pBuffer );
		if( pRes->pInfo ) Mem_Free( pRes->pInfo );
		Mem_Free( pRes );
	}

	return !found_problem;
}

void SV_CreateCustomizationList( sv_client_t *cl )
{
	resource_t	*pRes;
	customization_t	*pCust, *pNewCust;
	int		duplicated, lump_count;

	cl->customization.pNext = NULL;

	for( pRes = cl->resource1.pNext; pRes != &cl->resource1; pRes = pRes->pNext )
	{
		duplicated = false;

		for( pCust = cl->customization.pNext; pCust != NULL; pCust = pCust->pNext )
		{
			if( !Q_memcmp( pCust->resource.rgucMD5_hash, pRes->rgucMD5_hash, 16 ))
			{
				duplicated = true;
				break;
			}
		}

		if( duplicated )
		{
			MsgDev( D_WARN, "CreateCustomizationList: duplicate resource detected.\n" );
			continue;
		}

		// create it.
		lump_count = 0;

		if( !SV_CreateCustomization( &cl->customization, pRes, -1, 3, &pNewCust, &lump_count ))
		{
			MsgDev( D_WARN, "CreateCustomizationList: ignoring custom decal.\n" );
			continue;
		}

		pNewCust->nUserData2 = lump_count;
		svgame.dllFuncs.pfnPlayerCustomization( cl->edict, pNewCust );
	}
}

void SV_ClearCustomizationList( customization_t *pHead )
{
	customization_t	*pNext, *pCur;
	cachewad_t	*wad;

	if( !pHead || !pHead->pNext )
		return;

	pCur = pHead->pNext;

	do
	{
		pNext = pCur->pNext;

		if( pCur->bInUse && pCur->pBuffer )
			Mem_Free( pCur->pBuffer );

		if( pCur->bInUse && pCur->pInfo )
		{
			if( pCur->resource.type == t_decal )
			{
				wad = (cachewad_t *)pCur->pInfo; 
				Mem_Free( wad->filename );
				FS_Close( wad->file );
			}

			Mem_Free( pCur->pInfo );
		}

		Mem_Free( pCur );
		pCur = pNext;
	} while( pCur != NULL );

	pCur->pNext = NULL;
}

qboolean SV_FileInConsistencyList( resource_t *resource, sv_consistency_t **out )
{
	int i = 0;

	while( 1 )
	{
		if( !sv.consistency_files[i].name )
			return false; // end of consistency res

		if( !Q_stricmp( sv.consistency_files[i].name, resource->szFileName ))
			break;
		i++;
	}

	// get pointer on a matched record
	if( out ) *out = &sv.consistency_files[i];

	return true;
}

int SV_TransferConsistencyInfo( void )
{
	sv_consistency_t	*check;
	string		filepath;
	vec3_t		mins, maxs;
	int		i, total = 0;
	resource_t	*res;

	for( i = 0; i < sv.num_resources; i++ )
	{
		res = &sv.resources[i];

		if( res->ucFlags & RES_CHECK_CONSISTENCY )
			continue;	// already checked?

		if( !SV_FileInConsistencyList( res, &check ))
			continue;

		res->ucFlags |= RES_CHECK_CONSISTENCY;

		if( res->type == t_sound )
			Q_snprintf( filepath, sizeof( filepath ), "sound/%s", res->szFileName );
		else Q_strncpy( filepath, res->szFileName, sizeof( filepath ));

		MD5_HashFile( res->rgucMD5_hash, filepath, NULL );

		if( res->type == t_model )
		{
			switch( check->force_state )
			{
			case force_model_specifybounds_if_avail:
			case force_exactfile:
				// only MD5 hash compare
				break;
			case force_model_samebounds:
				if( !Mod_GetStudioBounds( filepath, mins, maxs ))
					Host_Error( "Mod_GetStudioBounds: couldn't get bounds for %s\n", filepath );

				res->rguc_reserved[0x00] = check->force_state;
				Q_memcpy( &res->rguc_reserved[0x01], mins, sizeof( mins ));
				Q_memcpy( &res->rguc_reserved[0x0D], maxs, sizeof( maxs ));
				break;
			case force_model_specifybounds:
				res->rguc_reserved[0x00] = check->force_state;
				Q_memcpy( &res->rguc_reserved[0x01], check->mins, sizeof( check->mins ));
				Q_memcpy( &res->rguc_reserved[0x0D], check->maxs, sizeof( check->maxs ));
				break;
			}
		}
		total++;
	}

	return total;
}

void SV_SendConsistencyList( sizebuf_t *msg )
{
	resource_t	*res;
	int		resIndex;
	int		lastIndex;
	int		i;

	if( mp_consistency->integer && ( sv.num_consistency_resources > 0 ) && !svs.currentPlayer->hltv_proxy )
	{
		lastIndex = 0;
		BF_WriteOneBit( msg, 1 );

		for( i = 0; i < sv.num_resources; i++ )
		{
			res = &sv.resources[i];

			if(!( res->ucFlags & RES_CHECK_CONSISTENCY ))
				continue;

			resIndex = (i - lastIndex);
			BF_WriteOneBit( msg, 1 );
			BF_WriteSBitLong( msg, resIndex, MAX_MODEL_BITS );
			lastIndex = i;
		}
	}

	// write end of the list
	BF_WriteOneBit( msg, 0 );
}
   
void SV_SendResources( sizebuf_t *msg )
{
	byte	nullrguc[32];
	int	i;

	Q_memset( nullrguc, 0, sizeof( nullrguc ));

	BF_WriteByte( msg, svc_customization );
	BF_WriteLong( msg, svs.spawncount );

	// g-cont. This is more than HL limit but unmatched with GoldSrc protocol
	BF_WriteSBitLong( msg, sv.num_resources, MAX_MODEL_BITS );

	for( i = 0; i < sv.num_resources; i++ )
	{
		BF_WriteSBitLong( msg, sv.resources[i].type, 4 );
		BF_WriteString( msg, sv.resources[i].szFileName );
		BF_WriteSBitLong( msg, sv.resources[i].nIndex, MAX_MODEL_BITS );
		BF_WriteSBitLong( msg, sv.resources[i].nDownloadSize, 24 );	// prevent to download a very big files?
		BF_WriteSBitLong( msg, sv.resources[i].ucFlags, 3 );	// g-cont. why only first three flags?

		if( sv.resources[i].ucFlags & RES_CUSTOM )
		{
			BF_WriteBits( msg, sv.resources[i].rgucMD5_hash, sizeof( sv.resources[i].rgucMD5_hash ));
		}

		if( Q_memcmp( nullrguc, sv.resources[i].rguc_reserved, sizeof( nullrguc )))
		{
			BF_WriteOneBit( msg, 1 );
			BF_WriteBits( msg, sv.resources[i].rguc_reserved, sizeof( sv.resources[i].rguc_reserved ));

		}
		else
		{
			BF_WriteOneBit( msg, 0 );
		}
	}

	SV_SendConsistencyList( msg );
}
