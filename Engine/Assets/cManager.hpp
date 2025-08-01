#ifndef EAE6320_ASSETS_CMANAGER_INL
#define EAE6320_ASSETS_CMANAGER_INL

// Includes
//=========

#include "cManager.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <limits>

// Interface
//==========

// Access
//-------

	template <class tAsset>
tAsset* sca2025::Assets::cManager<tAsset>::Get( const cHandle<tAsset> i_handle )
{
	EAE6320_ASSERTF( i_handle, "This handle is invalid (it has never been associated with a valid asset)" );
	// Lock the collections
	Concurrency::cMutex::cScopeLock autoLock( m_mutex );
	{
		const auto index = i_handle.GetIndex();
		const auto assetCount = m_assetRecords.size();
		if ( index < assetCount )
		{
			auto& assetRecord = m_assetRecords[index];
			const auto id_assetRecord = assetRecord.id;
			const auto id_handle = i_handle.GetId();
			if ( id_handle == id_assetRecord )
			{
				return assetRecord.asset;
			}
			else
			{
				EAE6320_ASSERTF( false, "A handle (at index %u) has an ID (%u) that doesn't match the asset record (%u)",
					index, id_handle, id_assetRecord );
			}
		}
		else
		{
			EAE6320_ASSERTF( false, "A handle has an index (%u) that's too big for the number of assets (%u)",
				index, assetCount );
		}
	}
	// If this code is reached the handle doesn't point to a valid asset
	return nullptr;
}

// Initialization / Clean Up
//--------------------------

	template <class tAsset> template <typename... tConstructorArguments>
sca2025::cResult sca2025::Assets::cManager<tAsset>::Load( const char* const i_path, cHandle<tAsset>& o_handle, tConstructorArguments&&... i_constructorArguments )
{
	// Get the existing asset if the path has already been loaded
	{
		// Lock the collections
		Concurrency::cMutex::cScopeLock autoLock( m_mutex );
		{
			auto iterator = m_map_pathsToHandles.find( i_path );
			if ( iterator != m_map_pathsToHandles.end() )
			{
				// Even if an entry exists it may no longer be valid
				// (the map doesn't get cleared when an asset is deleted)
				const auto existingHandle = iterator->second;
				const auto index = existingHandle.GetIndex();
				const auto assetCount = m_assetRecords.size();
				if ( index < assetCount )
				{
					auto& assetRecord = m_assetRecords[index];
					const auto id_assetRecord = assetRecord.id;
					const auto id_handle = existingHandle.GetId();
					if ( id_handle == id_assetRecord )
					{
						EAE6320_ASSERT( assetRecord.asset );
						const auto referenceCount = assetRecord.referenceCount;
						if ( referenceCount < std::numeric_limits<decltype( assetRecord.referenceCount )>::max() )
						{
							assetRecord.referenceCount = referenceCount + 1;
							o_handle = existingHandle;
							return Results::Success;
						}
						else
						{
							EAE6320_ASSERTF( false,
								"The asset \"%s\" has been loaded too many times (the manager's reference count is too big)", i_path );
							Logging::OutputError( "A new instance of \"%s\" couldn't be loaded because the manager's reference count was too big", i_path );
							return Results::Failure;
						}
					}
				}
				// If this code is reached it means that the existing entry is invalid
				m_map_pathsToHandles.erase( iterator );
			}
		}
	}

	// If the asset hasn't already been loaded load it now
	auto result = Results::Success;

	tAsset* newAsset;
	if ( result = tAsset::Load( i_path, newAsset, std::forward<tConstructorArguments>( i_constructorArguments )... ) )
	{
		// Lock the collections
		Concurrency::cMutex::cScopeLock autoLock( m_mutex );
		{
			// Look for an existing asset record that is unused
			if ( !m_unusedAssetRecordIndices.empty() )
			{
				const auto index = m_unusedAssetRecordIndices.back();
				{
					m_unusedAssetRecordIndices.pop_back();
				}
				auto& assetRecord = m_assetRecords[index];
				{
					assetRecord.asset = newAsset;
					assetRecord.referenceCount = 1;
				}
				o_handle = cHandle<tAsset>( index, assetRecord.id );
			}
			else
			{
				// Create a new asset record
				const auto assetRecordCount = m_assetRecords.size();
				if ( assetRecordCount < cHandle<tAsset>::InvalidIndex )
				{
					constexpr uint16_t id = 0;
					{
						constexpr uint16_t referenceCount = 1;
						m_assetRecords.push_back( sAssetRecord( newAsset, id, referenceCount ) );
					}
					{
						const auto index = static_cast<uint_fast32_t>( assetRecordCount );
						o_handle = cHandle<tAsset>( index, id );
					}
				}
				else
				{
					result = Results::OutOfMemory;
					EAE6320_ASSERTF( false, "Too many of this kind of asset have been created" );
					Logging::OutputError( "A new asset couldn't be loaded because there were too many (%u)", assetRecordCount );
				}
			}
			if ( result )
			{
				m_map_pathsToHandles.insert( std::make_pair( i_path, o_handle ) );
			}
		}
	}

	if ( !result && newAsset )
	{
		newAsset->DecrementReferenceCount();
		newAsset = nullptr;
	}

	return result;
}

	template <class tAsset>
sca2025::cResult sca2025::Assets::cManager<tAsset>::Release( cHandle<tAsset>& o_handle )
{
	auto result = Results::Success;

	EAE6320_ASSERTF( o_handle, "This handle is invalid (it has never been associated with a valid asset)" );
	// Lock the collections
	{
		Concurrency::cMutex::cScopeLock autoLock( m_mutex );
		{
			const auto index = o_handle.GetIndex();
			const auto assetCount = m_assetRecords.size();
			if ( index < assetCount )
			{
				auto& assetRecord = m_assetRecords[index];
				const auto id_assetRecord = assetRecord.id;
				const auto id_handle = o_handle.GetId();
				if ( id_handle == id_assetRecord )
				{
					// Decrement the manager's reference count
					EAE6320_ASSERT( assetRecord.referenceCount > 0 );
					const auto newReferenceCount = --assetRecord.referenceCount;
					if ( newReferenceCount == 0 )
					{
						// If the manager's reference count is zero it means that
						// every client that has asked to load the asset has now released it,
						// and the manager can free the asset itself
						{
							EAE6320_ASSERT( assetRecord.asset );
							assetRecord.asset->DecrementReferenceCount();
						}
						// The existing asset record has already been allocated,
						// and can be re-used for a new asset
						{
							assetRecord.asset = nullptr;
							assetRecord.id = static_cast<uint16_t>( cHandle<tAsset>::IncrementId( id_assetRecord ) );
							m_unusedAssetRecordIndices.push_back( index );
						}
					}
				}
				else
				{
					EAE6320_ASSERTF( false, "A handle attempting to be released (at index %u) has an ID (%u) that doesn't match the asset record (%u)",
						index, id_handle, id_assetRecord );
					result = Results::Failure;
				}
			}
			else
			{
				EAE6320_ASSERTF( false, "A handle attempting to be released has an index (%u) that's too big for the number of assets (%u)",
					index, assetCount );
				result = Results::Failure;
			}
		}
	}
	// Make the handle invalid so that the caller no longer has access to an asset it doesn't hold a reference to
	o_handle.MakeInvalid();

	return result;
}

	template <class tAsset>
sca2025::cResult sca2025::Assets::cManager<tAsset>::Initialize()
{
	return Results::Success;
}

	template <class tAsset>
sca2025::cResult sca2025::Assets::cManager<tAsset>::CleanUp()
{
	auto result = Results::Success;

	{
		bool wereThereStillAssets = false;

		// Lock the collections
		{
			Concurrency::cMutex::cScopeLock autoLock( m_mutex );
			{
				for ( auto& assetRecord : m_assetRecords )
				{
					if ( assetRecord.asset )
					{
						EAE6320_ASSERTF( false, "A manager still has a record of an asset that hasn't been released" );
						result = Results::Failure;
						wereThereStillAssets = true;
						// The asset's reference count could be decremented until it gets destroyed,
						// but there's no way of knowing that the asset still isn't being used
						// and so the asset will leak
						assetRecord.asset = nullptr;
						// The following shouldn't be necessary since the manager is being cleaned up,
						// but it doesn't hurt to be safe
						assetRecord.id = cHandle<tAsset>::IncrementId( assetRecord.id );
						assetRecord.referenceCount = 0;
					}
				}

				m_assetRecords.clear();
				m_unusedAssetRecordIndices.clear();
				m_map_pathsToHandles.clear();
			}
		}

		if ( wereThereStillAssets )
		{
			Logging::OutputError( "A manager still had asset records while it was being cleaned up" );
		}
	}

	return result;
}

	template <class tAsset>
sca2025::Assets::cManager<tAsset>::~cManager<tAsset>()
{
	const auto result = CleanUp();
	EAE6320_ASSERT( result );
}

// Implementation
//===============

	template <class tAsset>
sca2025::Assets::cManager<tAsset>::sAssetRecord::sAssetRecord( tAsset* const i_asset, const uint16_t i_id, const uint16_t i_referenceCount )
	:
	asset( i_asset ), id( i_id ), referenceCount( i_referenceCount )
{

}

#endif	// EAE6320_ASSETS_CMANAGER_INL
