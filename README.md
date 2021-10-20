#SeismicUnixSQLite
Geophysicists, who have worked with huge SEGY seismic data, which often go from several hundreds GB to few TB, know very well the challenge, how to read out and sort desired sesimic sections for presentation/visualization. As a crucial part of the new in-house processing package SPRINT of READ Well Services, I made a solution based on the well known database sqlite3.  
This C++ software package consists essentially of 2 programs with helper classes: spdbwrite & spdbread. The former reads selected SEGY/SU trace headers and creates a sqlite database; the latter selects and sorts the traces according to input keyword specification and output a data stream in Seismic Unix format (segy traces without EBIDIC/tape headers).

More detailed documentation will follow soon......