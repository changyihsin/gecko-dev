/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsSoftwareUpdate.h"
#include "nsXPInstallManager.h"
#include "nsInstallTrigger.h"
#include "nsInstallVersion.h"
#include "nsIDOMInstallTriggerGlobal.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIScriptGlobalObject.h"

#include "nsIPref.h"

#include "nsRepository.h"
#include "nsIServiceManager.h"

#include "nsSpecialSystemDirectory.h"

#include "VerReg.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);

static NS_DEFINE_IID(kIInstallTrigger_IID, NS_IDOMINSTALLTRIGGERGLOBAL_IID);
static NS_DEFINE_IID(kIInstallTrigger_CID, NS_SoftwareUpdateInstallTrigger_CID);





nsInstallTrigger::nsInstallTrigger()
{
    mScriptObject   = nsnull;
    NS_INIT_ISUPPORTS();
}

nsInstallTrigger::~nsInstallTrigger()
{
}

NS_IMETHODIMP 
nsInstallTrigger::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
    if (aInstancePtr == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    // Always NULL result, in case of failure
    *aInstancePtr = NULL;

    if ( aIID.Equals(kIScriptObjectOwnerIID))
    {
        *aInstancePtr = (void*) ((nsIScriptObjectOwner*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kIInstallTrigger_IID) )
    {
        *aInstancePtr = (void*) ((nsIDOMInstallTriggerGlobal*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kISupportsIID) )
    {
        *aInstancePtr = (void*)(nsISupports*)(nsIScriptObjectOwner*)this;
        AddRef();
        return NS_OK;
    }

     return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsInstallTrigger)
NS_IMPL_RELEASE(nsInstallTrigger)



NS_IMETHODIMP 
nsInstallTrigger::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    NS_PRECONDITION(nsnull != aScriptObject, "null arg");
    nsresult res = NS_OK;
    
    if (nsnull == mScriptObject) 
    {
         nsIScriptGlobalObject *global = aContext->GetGlobalObject();

        res = NS_NewScriptInstallTriggerGlobal(  aContext, 
                                                (nsISupports *)(nsIDOMInstallTriggerGlobal*)this, 
                                                (nsISupports *)global, 
                                                &mScriptObject);
        NS_IF_RELEASE(global);

    }
  

    *aScriptObject = mScriptObject;
    return res;
}

NS_IMETHODIMP 
nsInstallTrigger::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

static NS_DEFINE_IID(kPrefsIID, NS_IPREF_IID);
static NS_DEFINE_IID(kPrefsCID,  NS_PREF_CID);

NS_IMETHODIMP    
nsInstallTrigger::UpdateEnabled(PRBool* aReturn)
{
    nsIPref * prefs;
    
    nsresult rv = nsServiceManager::GetService(kPrefsCID, 
                                               kPrefsIID,
                                               (nsISupports**) &prefs);


    if ( NS_SUCCEEDED(rv) )
    {
        rv = prefs->GetBoolPref( (const char*) XPINSTALL_ENABLE_PREF, aReturn);

        if (NS_FAILED(rv))
        {
            *aReturn = PR_FALSE;
        }

        NS_RELEASE(prefs);
    }
    else
    {
        *aReturn = PR_FALSE;  /* no prefs manager.  set to false */
    }

    return NS_OK;
}

NS_IMETHODIMP
nsInstallTrigger::Install(nsXPITriggerInfo* aTrigger, PRBool* aReturn)
{
    NS_ASSERTION(aReturn, "Invalid pointer arg");
    *aReturn = PR_FALSE;

    PRBool enabled;
    nsresult rv = UpdateEnabled(&enabled);
    if (NS_FAILED(rv) || !enabled) 
    {
        delete aTrigger;
        return NS_OK;
    }

    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        // The Install manager will delete itself when done
        rv = mgr->InitManager( aTrigger, 0 );
        if (NS_SUCCEEDED(rv))
            *aReturn = PR_TRUE;
    }
    else
    {
        delete aTrigger;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }


    return rv;
}


NS_IMETHODIMP
nsInstallTrigger::InstallChrome(PRUint32 aType, nsXPITriggerItem *aItem, PRBool* aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);
    NS_ENSURE_ARG_POINTER(aItem);
    *aReturn = PR_FALSE;


    // make sure we're allowing installs
    PRBool enabled;
    nsresult rv = UpdateEnabled(&enabled);
    if (NS_FAILED(rv) || !enabled)
        return NS_OK;


    // The Install manager will delete itself when done, once we've called
    // InitManager. Before then **WE** must delete it
    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        nsXPITriggerInfo* trigger = new nsXPITriggerInfo();
        if ( trigger )
        {
            trigger->Add( aItem );

            // The Install manager will delete itself when done
            rv = mgr->InitManager( trigger, aType );
            *aReturn = PR_TRUE;
        }
        else
        {
            rv = NS_ERROR_OUT_OF_MEMORY;
            delete mgr;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::StartSoftwareUpdate(const nsString& aURL, PRInt32 aFlags, PRBool* aReturn)
{
    PRBool enabled;
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    *aReturn = PR_FALSE;

    UpdateEnabled(&enabled);
    if (!enabled)
        return NS_OK;

    // The Install manager will delete itself when done, once we've called
    // InitManager. Before then **WE** must delete it
    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        nsXPITriggerInfo* trigger = new nsXPITriggerInfo();
        if ( trigger )
        {
            nsXPITriggerItem* item = new nsXPITriggerItem(0,aURL.GetUnicode());
            if (item)
            {
                trigger->Add( item );
                // The Install manager will delete itself when done
                rv = mgr->InitManager( trigger, 0 );
                *aReturn = PR_TRUE;
            }
            else
            {
                rv = NS_ERROR_OUT_OF_MEMORY;
                delete trigger;
                delete mgr;
            }
        }
        else
        {
            rv = NS_ERROR_OUT_OF_MEMORY;
            delete mgr;
        }
    }

    return rv;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, PRInt32 aDiffLevel, const nsString& aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aVersion);
    return ConditionalSoftwareUpdate(aURL, aRegName, aDiffLevel, &inVersion, aMode, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, PRInt32 aDiffLevel, nsIDOMInstallVersion* aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    PRBool needJar = PR_FALSE;

    PRBool enabled;

    UpdateEnabled(&enabled);
    if (!enabled)
        return NS_OK;

    if (aURL.IsEmpty() || aVersion == nsnull)
    {
        needJar = PR_TRUE;
    }
    else
    {
        char * regNameCString = aRegName.ToNewCString();

        REGERR status = VR_ValidateComponent( regNameCString );
        
        if ( status == REGERR_NOFIND || status == REGERR_NOFILE )
        {
            // either component is not in the registry or it's a file
            // node and the physical file is missing
            needJar = PR_TRUE;
        }
        else
        {
            VERSION oldVersion;
            PRInt32 diffValue;

            status = VR_GetVersion( regNameCString, &oldVersion );
            nsInstallVersion oldInstallVersion;

            oldInstallVersion.Init(oldVersion.major, 
                                oldVersion.minor, 
                                oldVersion.release, 
                                oldVersion.build);


            if ( status != REGERR_OK )
                needJar = PR_TRUE;
            else if ( aDiffLevel < 0 )
            {
                aVersion->CompareTo(&oldInstallVersion, &diffValue); 
                needJar = (diffValue <= aDiffLevel);
            }
            else
            {
                aVersion->CompareTo(&oldInstallVersion, &diffValue);
                needJar = (diffValue >= aDiffLevel);
            }
        }
    }

    if (needJar)
        return StartSoftwareUpdate(aURL, aMode, aReturn);
    else
        *aReturn = 0;

    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    return ConditionalSoftwareUpdate(aURL, aRegName, BLD_DIFF, aVersion, aMode, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, const nsString& aVersion, PRInt32 aMode, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aVersion);
    return ConditionalSoftwareUpdate(aURL, aRegName, BLD_DIFF, &inVersion, aMode, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aVersion);;
    return ConditionalSoftwareUpdate(aURL, aRegName, BLD_DIFF, &inVersion, 0, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::ConditionalSoftwareUpdate(const nsString& aURL, const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    return ConditionalSoftwareUpdate(aURL, aRegName, BLD_DIFF, aVersion, 0, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aMajor, aMinor, aRelease, aBuild);

    return CompareVersion(aRegName, &inVersion, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aVersion);

    return CompareVersion(aRegName, &inVersion, aReturn);
}

NS_IMETHODIMP    
nsInstallTrigger::CompareVersion(const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    *aReturn = EQUAL;  // assume failure.

    PRBool enabled;

    UpdateEnabled(&enabled);
    if (!enabled)
        return NS_OK;

    VERSION              cVersion;
    char*                tempCString;
    REGERR               status;
    nsInstallVersion     regNameVersion;
    
    tempCString = aRegName.ToNewCString();

    status = VR_GetVersion( tempCString, &cVersion );

    /* if we got the version */
    if ( status == REGERR_OK ) 
    {
        if ( VR_ValidateComponent( tempCString ) == REGERR_NOFILE ) 
        {
            regNameVersion.Init(0,0,0,0);
        }
        else
        {
            regNameVersion.Init(cVersion.major, 
                                cVersion.minor, 
                                cVersion.release, 
                                cVersion.build);
        }
    }
    else
        regNameVersion.Init(0,0,0,0);
        
    regNameVersion.CompareTo( aVersion, aReturn );

    if (tempCString)
        Recycle(tempCString);
    
    return NS_OK;
}

NS_IMETHODIMP    
nsInstallTrigger::GetVersion(const nsString& component, nsString& version)
{
    PRBool enabled;

    UpdateEnabled(&enabled);
    if (!enabled)
        return NS_OK;

    VERSION              cVersion;
    char*                tempCString;
    REGERR               status;
    
    tempCString = component.ToNewCString();

    status = VR_GetVersion( tempCString, &cVersion );

    version.Truncate();

    /* if we got the version */
    if ( status == REGERR_OK && VR_ValidateComponent( tempCString ) == REGERR_OK) 
    {
        nsInstallVersion regNameVersion;
        
        regNameVersion.Init(cVersion.major, 
                            cVersion.minor, 
                            cVersion.release, 
                            cVersion.build);

        regNameVersion.ToString(version);
    }
    
    if (tempCString)
         Recycle(tempCString);

    return NS_OK;
}


// this will take a nsIURI, and create a temporary file.  If it is local, we just us it.
 
void
nsInstallTrigger::CreateTempFileFromURL(const nsString& aURL, nsString& tempFileString)
{
    // Checking to see if the url is local

    if ( aURL.EqualsIgnoreCase("file:/", 6) )
    {       
        tempFileString.AssignWithConversion( NS_STATIC_CAST(const char*, nsNSPRPath(nsFileURL(aURL))) );
    }
    else
    {
        nsSpecialSystemDirectory tempFile(nsSpecialSystemDirectory::OS_TemporaryDirectory);
    
        PRInt32 result = aURL.RFindChar('/');
        if (result != -1)
        {    
            nsString jarName;
                       
            aURL.Right(jarName, (aURL.Length() - result) );
            
            PRInt32 argOffset = jarName.RFindChar('?');

            if (argOffset != -1)
            {
                // we need to remove ? and everything after it
                jarName.Truncate(argOffset);
            }
            
            
            tempFile += jarName;
        }
        else
        {   
            tempFile += "xpinstall.jar";
        }

        tempFile.MakeUnique();

        tempFileString.AssignWithConversion( NS_STATIC_CAST(const char*, nsNSPRPath( nsFilePath(tempFile) )) );
    }
}


