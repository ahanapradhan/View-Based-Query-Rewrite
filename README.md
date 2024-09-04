


# Setting Up the Database
## PostgreSQL Installation  

Follow the link https://www.postgresql.org/download/ to download and install the suitable distribution of the database for your platform. 

## Loading TPCH Data using DBGEN

### Obtaining DBGEN
1. Open the TPC webpage following the link: https://www.tpc.org/tpc_documents_current_versions/current_specifications5.asp  
2. In the `Active Benchmarks` table (first table), follow the link of `Download TPC-H_Tools_v3.0.1.zip`, it'll redirect to `TPC-H Tools Download
` page   
3. Give your details and click `download`, it'll email you the download link. Use the link to download the zip file.  
4. Unzip the zip file, and it must have the `dbgen` folder among the extracted contents  

### Prepare TPCH data on PostgreSQL using DBGEN
1. Download the code `tpch-pgsql` from the link: [https://github.com/Data-Science-Platform/tpch-pgsql/tree/master](https://github.com/ahanapradhan/tpch-pgsql).  
2. Follow the `tpch-pgsql` project Readme to prepare and load the data.  
3. (In case the above command gives error as `malloc.h` not found, showing the filenames, go inside dbgen folder, open the file and replace `malloc.h` with `stdlib.h`)

## Sample TPCH Data  
TPCH 100MB (sf=0.1) data is provided at: https://github.com/ahanapradhan/UnionExtraction/blob/master/mysite/unmasque/test/experiments/data/tpch_tiny.zip  
The load.sql file in the folder needs to be updated with the corresponding location of the data .csv files.

## Loading TPCH Data using DuckDB (Easiest Way)
https://duckdb.org/docs/extensions/tpch.html
This approach is the fastest. However, your machine should have enough space to install DuckDB first!
