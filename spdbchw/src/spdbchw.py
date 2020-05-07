#!/usr/bin/env python

import sys
import os
#from pysqlite2 import dbapi2 as sqlite3
from math import *

class commandLine(object):

  def __init__(self):
    self.evals1 = {}
    self.evals2 = {}
    self.dbpath = ""
    self.dbout = ""
    
    for arg in sys.argv:
      sl = arg.split("=")
      if len(sl) < 2: continue
      k = sl[0].strip()
      v = sl[1].strip()
      if k == "dbpath": self.dbpath = v
      elif k == "dbout": self.dbout = v
      elif k.startswith("@"): self.evals1[k[1:]] = compile(v, "error.pc", "eval")  
      else: self.evals2[k] = compile(v, "error.pc", "eval")  

  def transform(self, input):
    run = {}
    run.update(input)
    for p in [self.evals1, self.evals2]:
      me = {}
      for k, v in p.items():
        me[k] = eval(v, globals(), run)
      run.update(me)
    input.update(me)
    return input
  
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
      if self.types[k] == "integer": vv += str(int(v))
      else: vv += str(v)
      
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
    
class tableCopy(object):
  
  def __init__(self, dbname, dbout):
    self.dbname = dbname
    self.dbout = dbout
    if dbout == "": self.dbout = dbname
    elif os.path.exists(dbout): 
      print >>sys.stderr, dbout, "already exists, please specify a different dbout file"
      sys.exit()
    
  def copy(self, trans):
    headers = table(self.dbname, "headers")
    old = headers.rows()
    if len(old) == 0: return
    
    cs = trans.transform(headers.bind(old[0]))
    t = {}
    t.update(headers.types)
    for k in cs.iterkeys():
      if not t.has_key(k):
        t[k] = "double"
    nheaders = table(self.dbout, "nheaders", t)

    con = sqlite3.connect(self.dbout)
    cursor = con.cursor()

    i = 0
    for d in old:
      cs = trans.transform(headers.bind(d))
      nheaders.insert(cursor, cs)
      ++i
      if i > 10000:
        i = 0 
        con.commit()

    con.commit()
    cursor.close()
    con.close()

    if self.dbname == self.dbout:
      headers.execute("drop table headers")
      headers.execute("alter table nheaders rename to headers")
    else:
      m = headers.readall("select key, value from meta")
      nheaders.execute("create table meta (key string primary key, value string)")
      for i in m:
        sql = "insert into meta (key, value) values ('" + str(i[0]) + "', '" + str(i[1]) + "')"
        nheaders.execute(sql) 
      nheaders.execute("alter table nheaders rename to headers")
      
cl = commandLine()
if cl.dbpath != "":
  tableCopy(cl.dbpath, cl.dbout).copy(cl)
else:
  print """
SUDoc for SPDBCHW

SPDBCHW change header fields of the index db 

 spdbchw dbpath= [optional parameters]

 Required parameter:

      dbpath= Path to the file that contains the SQLite database
             indexing the input stream. The file usually has the
             extension ".db". The file must already exist,
             otherwise this module exits with an error code. See 
             module SPDBWRITE for more details.

 Optional parameters:

      dbout= Path to the output database. If this parameter is
             missing, the original database is updated. Otherwise
             the headers table of the new database contains all 
             the new values and all the original values of the 
             columns that have not changed in the original database,
             plus the entire meta table will be copied from the 
             original database.
      [result spec]=[value expression]: calculate a new value based 
             on existing values and literal constants. The existing 
             values can be from the database or from intermediate
             results. Some of the result can be used to override 
             the column values in the database. The expressions are
             directly interpreted as Python expressions. More details
             about evaluation rules are discussed below.

 Evaluation rules:

      The module reads each row from the database and evaluates new
      values according to the parameters given in the command line. 
      Some of these new values are used to update the data in the 
      same row. All rows are processed identically. For each row, 
      the following steps take place:

        1. Values of all columns in the row are read. They are 
           given names identical to their column names.
      
        2. All names of the command line parameters where the 
           name starts with '@' have their value expressions 
           evaluated. The results are given the name of the 
           parameter without the leading '@'. 

        3. All names of the command line parameters where the 
           name does not start with '@', except the one with 
           the name "dbpath" and "dbout", have their value
           expressions evaluated. The results are attached to 
           the row with the name of the parameter. If the name
           of a value is identical to a column in the table, the
           corresponding value in the row will be replaced by the
           new value. After all values are ready for a row, that              
           row will either be used to update the original database
           or to be inserted into the new database (as given by 
           the parameter "dbout". In case a new column have to be
           created, its db type is "double".

        4. During the evaluation of any expression, all values
           produced in previous steps can be used by referring
           to them by their names. If the result name of an 
           expression is identical to the name of an existing
           value, that value is overridden for later steps.

 Examples:

    calculate offset and azimuth in floating point
        spdbchw dbpath=mydata.db  \\
    "@dxm=(sx-gx)/10" @dym="(sy-gy)/10" \\
    foffset="sqrt(dxm**2 + dym**2)" \\
    azimuth="atan(dym/dxm)"
  
    rotate coordinate clockwise by 35.5 degrees 
        spdbchw dbpath=mydata.db dbout=mydata+rotated.db \\
    @f="2*3.1416/360" @xo=4700.5 @yo=-234.7 @angle=-35.5 \\
    sx=" cos(angle*f)*(sx-xo) + sin(angle*f)*(sy-yo)" \\
    sy="-sin(angle*f)*(sx-xo) + cos(angle*f)*(sy-yo)" \\
    gx=" cos(angle*f)*(gx-xo) + sin(angle*f)*(gy-yo)" \\
    gy="-sin(angle*f)*(gx-xo) + cos(angle*f)*(gy-yo)"
 """
