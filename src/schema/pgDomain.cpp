//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// Copyright (C) 2002, The pgAdmin Development Team
// This software is released under the pgAdmin Public Licence
//
// pgDomain.cpp - Domain class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"
#include "pgObject.h"
#include "pgDomain.h"
#include "pgCollection.h"


pgDomain::pgDomain(pgSchema *newSchema, const wxString& newName)
: pgSchemaObject(newSchema, PG_DOMAIN, newName)
{
}

pgDomain::~pgDomain()
{
}


wxString pgDomain::GetSql(wxTreeCtrl *browser)
{
    if (sql.IsNull())
    {
        sql = wxT("CREATE DOMAIN ") + GetQuotedFullIdentifier() 
            + wxT(" AS ") + GetBasetype();
        if (typlen == -1 && typmod > 0)
        {
            sql += wxT("(") + NumToStr(length);
            if (precision >= 0)
                sql += wxT(", ") + NumToStr(precision);
            sql += wxT(")");
        }
        if (dimensions)
        {
            sql += wxT("[");
            for (int i=0 ; i < dimensions ; i++)
            {
                if (i)
                    sql += GetDelimiter() + wxT(" ");
            }
            sql += wxT("]");
        }
        if (!GetDefault().IsNull())
            sql += wxT(" DEFAULT ") + qtString(GetDefault());
        // CONSTRAINT ....
        if (notNull)
            sql += wxT(" NOT NULL");


        sql += wxT(";\n")
            + GetCommentSql();
    }

    return sql;
}

void pgDomain::ShowTreeDetail(wxTreeCtrl *browser, frmMain *form, wxListCtrl *properties, wxListCtrl *statistics, ctlSQLBox *sqlPane)
{
    SetButtons(form);

    if (!expandedKids)
    {
        expandedKids = true;
        // append type here
    }
    properties->ClearAll();
    properties->InsertColumn(0, wxT("Property"), wxLIST_FORMAT_LEFT, 150);
    properties->InsertColumn(1, wxT("Value"), wxLIST_FORMAT_LEFT, 200);
  

    int pos=0;

    InsertListItem(properties, pos++, wxT("Name"), GetName());
    InsertListItem(properties, pos++, wxT("OID"), NumToStr(GetOid()));
    InsertListItem(properties, pos++, wxT("Owner"), GetOwner());
    InsertListItem(properties, pos++, wxT("Base Type"), GetBasetype());
    if (GetLength() == -2)
        InsertListItem(properties, pos++, wxT("Length"), wxT("null-terminated"));
    else
    {
        if (GetDimensions() || GetLength() < 0)
            InsertListItem(properties, pos++, wxT("Length"), wxT("variable"));
        else
            InsertListItem(properties, pos++, wxT("Length"), NumToStr(GetLength()));
    }
    if (GetPrecision() >= 0)
        InsertListItem(properties, pos++, wxT("Precision"), NumToStr(GetPrecision()));
    // dimensions, delimiter missing
    if (GetDimensions())
        InsertListItem(properties, pos++, wxT("Dimensions"), NumToStr(GetDimensions()));
    InsertListItem(properties, pos++, wxT("Default"), GetDefault());
    InsertListItem(properties, pos++, wxT("Not Null?"), BoolToYesNo(GetNotNull()));
    InsertListItem(properties, pos++, wxT("System Domain?"), BoolToYesNo(GetSystemObject()));
    InsertListItem(properties, pos++, wxT("Comment"), GetComment());
}



void pgDomain::ShowTreeCollection(pgCollection *collection, frmMain *form, wxTreeCtrl *browser, wxListCtrl *properties, wxListCtrl *statistics, ctlSQLBox *sqlPane)
{
    wxString msg;
    pgDomain *domain;

    if (browser->GetChildrenCount(collection->GetId(), FALSE) == 0)
    {
        // Log
        msg.Printf(wxT("Adding Domains to schema %s"), collection->GetSchema()->GetIdentifier().c_str());
        wxLogInfo(msg);

        // Add Domain node
//        pgObject *addDomainObj = new pgObject(PG_ADD_DOMAIN, wxString("Add Domain"));
//        browser->AppendItem(collection->GetId(), wxT("Add Domain..."), 4, -1, addDomainObj);

        // Get the Domains
        pgSet *domains= collection->GetDatabase()->ExecuteSet(wxT(
            "SELECT d.oid, d.typname as domname, d.typbasetype, b.typname as basetype, pg_get_userbyid(d.typowner) as domainowner, \n"
            "       d.typlen, d.typtypmod, d.typnotnull, d.typdefault, d.typndims, d.typdelim,\n"
            "       CASE WHEN d.oid=1700 OR d.typbasetype=1700 THEN 1 ELSE 0 END AS isnumeric\n"
            "  FROM pg_type d\n"
            "  JOIN pg_type b ON b.oid = CASE WHEN d.typndims>0 then d.typelem ELSE d.typbasetype END\n"
            " WHERE d.typtype = 'd' AND d.typnamespace = ") + NumToStr(collection->GetSchema()->GetOid()) + wxT("::oid\n"
            " ORDER BY d.typname"));

        if (domains)
        {
            while (!domains->Eof())
            {
                domain = new pgDomain(collection->GetSchema(), domains->GetVal(wxT("domname")));

                domain->iSetOid(StrToDouble(domains->GetVal(wxT("oid"))));
                domain->iSetOwner(domains->GetVal(wxT("domainowner")));
                domain->iSetBasetype(domains->GetVal(wxT("basetype")));
                domain->iSetBasetypeOid(StrToDouble(domains->GetVal(wxT("typbasetype"))));
                long typlen=StrToLong(domains->GetVal(wxT("typlen")));
                long typmod=StrToLong(domains->GetVal(wxT("typtypmod")));
                bool isnum=StrToBool(domains->GetVal(wxT("isnumeric")));
                long length=typlen, precision=-1;
                if (typlen == -1 && typmod > 0)
                {
                        if (isnum)
                        {
                            length=(typmod-4) >> 16;
                            precision=(typmod-4) & 0xffff;
                        }
                        else
                            length = typmod-4;
                }
                domain->iSetTyplen(typlen);
                domain->iSetTypmod(typmod);
                domain->iSetLength(length);
                domain->iSetPrecision(precision);
                domain->iSetDefault(domains->GetVal(wxT("typdefault")));
                domain->iSetNotNull(StrToBool(domains->GetVal(wxT("typnotnull"))));
                domain->iSetDimensions(StrToLong(domains->GetVal(wxT("typndims"))));
                domain->iSetDelimiter(domains->GetVal(wxT("typdelim")));

                browser->AppendItem(collection->GetId(), domain->GetIdentifier(), PGICON_DOMAIN, -1, domain);
	    
			    domains->MoveNext();
            }

		    delete domains;
        }
    }
}

