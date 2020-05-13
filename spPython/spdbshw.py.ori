#!/usr/bin/env python

import sys
import os
import shutil
import re
from sets import Set
from math import *
from pysqlite2 import dbapi2 as sqlite3

class commandLine(object):

  def __init__(self):
    params = {}
    for arg in sys.argv:
      sl = arg.split("=")
      if len(sl) < 2: continue
      k = sl[0].strip()
      v = sl[1].strip()
      params[k] = v
    
    self.dbpath = params.pop("dbpath", "")
    self.table = params.pop("table", "")
    self.matches = params.pop("matches", "").split(",")
    self.replaces = params.pop("replaces", "").split(",")
    self.dvalues = {}
    
    dvaluematch = re.compile("(.*)\((\d*)\)")
    for rep in self.replaces:
      m = dvaluematch.match(rep)
      if m:
        gs = m.groups()
        self.dvalues[gs[0]] = gs[1]
      else:
        self.dvalues[rep] = "0.0"
    
    dbout = params.pop("dbout", "")
    if not dbout == "":
      shutil.copy(self.dbpath, dbout)
      self.dbpath = dbout
  
  def tableTypes(self, known):
    res = {}
    for r in self.matches:
      res[r] = known[r]
    for r in self.replaces:
      res[r] = known.get(r, "double")
    return res
  
  def listColumns(self):
    return ",".join(self.matches) + "," + ",".join(self.replaces)
  
  def ready(self):
    return not self.dbpath == "" and \
           not self.table == "" and \
           len(self.matches) > 0 and \
           len(self.replaces) > 0
  
class table(object):
  def __init__(self, db, table, types = {}):
    self.db = db
    self.table = table
    self.types = types
    
    if len(types) == 0:
      cs = self.readall("PRAGMA table_info(" + self.table + ")")
      for i in cs:
        self.types[i[1]] = i[2]
    else:
      l = ""
      for k, v in self.types.items():
        if len(l) > 0:
          l += ", "
        l += k + " " + v
        if k == "indexnumber" or k == "key":
          l += " primary key"
      self.execute("drop table " + self.table)
      self.execute("create table " + self.table + "(" + l + ")")
    
    self.columns = []
    self.cnames = ""
    for k, v in self.types.items():
      self.columns.append(k)
      if len(self.cnames) > 0:
          self.cnames += ", "
      self.cnames += k

  def rows(self):
    return self.readall("select " + self.cnames + " from " + self.table)

  def bind(self, row):
    res = {}
    for i in range(len(row)):
      res[self.columns[i]] = row[i]
    return res
  
  def insert(self, cursor, data):
    kk = ""
    vv = ""
    for k, v in data.items():
      if len(kk) > 0:
          kk += ", "
          vv += ", "
      kk += k
      vv += str(v)
    cursor.execute("insert into " + self.table + " (" + kk + ") values (" + vv + ")")

  def readall(self, sql):
    con = sqlite3.connect(self.db)
    cursor = con.cursor()
    cursor.execute(sql)
    res = cursor.fetchall()
    cursor.close()
    con.close()
    return res

  def execute(self, sql):
    con = sqlite3.connect(self.db)
    cursor = con.cursor()
    try: 
      cursor.execute(sql)
      con.commit()
    except: pass
    cursor.close()
    con.close()
        
class dataCopy(object):
  def __init__(self, commandline):
    self.commandline = commandline
    
  def copy(self):
    dbname = self.commandline.dbpath
    columns = self.commandline.listColumns()
    
    headers = table(dbname, "headers")
    shw = self.commandline.tableTypes(headers.types)
    
    datatable = table(dbname, "data", shw)
    file = open(self.commandline.table)
    con = sqlite3.connect(dbname)
    cursor = con.cursor()

    i = 0
    for line in file:
      l = ",".join(line.split())
      cursor.execute("insert into data (" + columns + ") values (" + l + ")")
      ++i
      if i > 10000:
        i = 0 
        con.commit()

    con.commit()
    cursor.close()
    con.close()
    
    mats = ""
    vals1 = "h."
    
    hcols = Set(headers.columns)
    dcols = Set(self.commandline.replaces)
    ocols = hcols.difference(dcols)
    for c in ocols:
      vals1 += c + " " + c + ", h."
    vals1 = vals1[:-4]
    vals2 = vals1;
    
    for c in dcols:
      vals1 += ", d." + c + " " + c
      if c in hcols: vals2 += ", h." + c + " " + c
      else: vals2 += ", " + self.commandline.dvalues[c] + " " + c

    for c in self.commandline.matches:
      mats += "h." + c + " = d." + c + " and "
    mats = mats[:-5]
    
    sql = "create table nheaders as select " + vals1 + " from headers as h, data as d where " + \
          mats + " union select " + vals2 + " from headers h where not exists (select 1 from " +\
          " data as d where " +  mats + ")"
    
    headers.execute(sql)
    headers.execute("drop table headers")
    headers.execute("drop table data")
    headers.execute("alter table nheaders rename to headers")

cl = commandLine()
if cl.ready():
  dataCopy(cl).copy()
else:
  print """
SUDoc for SPDBSHW

SPDBSHW set header fields of the header table in the index db 

 spdbshw dbpath= table= matches= replaces= [optional parameters]

 Required parameter:

      dbpath= Path to the file that contains the sqlite database
             indexing some original data file. The file usually has 
             the extension ".db". The file must already exist,
             otherwise this module exits with an error code. See 
             module SPDBWRITE for more details.

      table=  Path to the ASCII file that contains all the data 
             necessary for setting of the header fields in the
             database. See discussions below for requirements of
             the content of this file.

      matches= The comma separated field list for selecting the 
             entry in the data file.

      replaces= The field list for the columns that need their 
             values replaced by the data contained in the data 
             file.

 Optional parameters:

      dbout= if specified, this is the file name for the resulting
             database. Otherwise, the original database is updated
             with the new data. If there is a new database file, 
             the meta table of the original database is copied 
             into the new database in its entirety.

 Content rules for the replacement data file:

      The following rules define the content of the data file
      containing the replacement data (datapath) in its 
      relationship to the "matches" list and "replaces" list.

        1. The file contains a (possibly large) number of lines,
           each contains a list of space separated numbers. All 
           lines contain the same number of values and that number
           is called number of columns in the file.

        2. The number of columns in the file equals exactly the 
           number of columns in the "matches" parameter plus that
           of the "replaces" parameter. The names of the columns
           in the file are assigned the names of the columns in 
           the parameter list in exactly that order. (The names
           do not appear in the datafile itself).

        3. Each of the column names in the "matches" list must 
           match the name of a column in the headers table of
           the database. 

        4. If the name of a column in the data file matches the
           name of a column in the database, all values in the
           data file for that column are regarded as having the
           same type as defined in the database. Otherwise the
           type of the column is regarded as "double" in sqlite.

        5. If the "replaces" column list contains names that does
           not exist as column name in the header table of the 
           database, a new column will be created for it. 

 Examples:

    load shotpoint navigation data to trace header in database
        spdbshw dbpath=rawdata.db dbout=data+shotgeom.db \\
    table=/navdata/shotgeom.txt \\
       matches=tracf,fldr replaces=sx,sy,swdep 

    load scaling parameters to table
        spdbshw dbpath=xyz.db table=scaling.txt \\
    matches=cdp replaces=unscale
 """
