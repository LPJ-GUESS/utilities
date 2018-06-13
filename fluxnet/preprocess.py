"""
This script is made to preprocess raw data from Fluxnet sites. It reads in zipped
raw-data and outputs .csv-files with forcing data to LPJ-GUESS and benchmark-data
for the post-processing script.

Written by: Adrian Gustafson
Date: 2018-06-12
"""
import os
from zipfile import ZipFile
from datetime import datetime

import pandas as pd


def format_dates(row):
    # Convert the Timestamp from double to string for slicing
    timestamp = str(row.pop('TIMESTAMP'))[:-2]
    row['Year'] = timestamp[:4]
    fmt = '%Y%m%d'
    
    if len(timestamp) == 6:
        # Monthly values
        row['Month'] = timestamp[4:]
    elif len(timestamp) == 8:
        # Daily values
        dt = datetime.strptime(timestamp, fmt)
        
        row['Month'] = timestamp[4:6]
        row['Day']   = timestamp[6:]
        row['MonthDay'] = timestamp[4:]
        # LPJ-GUESS day start at 0 but julian days start at 1
        row['DOY'] = int(dt.strftime('%j')) - 1
    else:
        # Should not happen
        print('invalid timeformat at timestamp {}'.format(timestamp))
        
    return row
   
"""
SETTINGS
""" 
BASE_DIR   = '/dauta/FLUXNET_from_Michael/fluxnet benchmark/'
INPUT_DIR  = 'raw'
OUTPUT_DIR = 'vut_ref'

sitelist_fn = 'fluxnet2015_sitelist.csv'

forcing_cols   = ['TA_ERA', 'SW_IN_ERA', 'P_ERA']
benchmark_cols = ['NEE_VUT_REF', 'GPP_NT_VUT_REF', 'LE_F_MDS']
input_cols = ['TIMESTAMP'] + forcing_cols + benchmark_cols

# Rename input variables according to this map.
column_map = {
    'TA_ERA': 'Temp',
    'SW_IN_ERA': 'Rad',
    'p_ERA': 'Prec',
    'NEE_VUT_REF': 'NEE',
    'GPP_NT_VUT_REF': 'GPP',
    'LE_F_MDS': 'LE',
    'Lon': 'Lon',
    'Lat': 'Lat',
    'IGBP': 'IGBP',
    'DOY': 'Day'
}

"""
SCRIPT STARTS
"""

sitelist = pd.read_csv(os.path.join(BASE_DIR, sitelist_fn), encoding = "ISO-8859-1")

daily = pd.DataFrame()
monthly = pd.DataFrame()
for fn in os.listdir(os.path.join(BASE_DIR, INPUT_DIR)):
    site = fn.split('_')[1]
    print('Extracting data from site {}'.format(site))
    site_data = sitelist[sitelist['SITE_ID'] == site]
    zf = ZipFile(os.path.join(BASE_DIR, INPUT_DIR, fn))
    for f in zf.namelist():
        if '_FULLSET_DD_' in f:
            daily_raw = f
            
        if '_FULLSET_MM_' in f:
            monthly_raw = f
        
    df_daily = pd.read_csv(zf.open(daily_raw), usecols=input_cols)    
    df_monthly = pd.read_csv(zf.open(monthly_raw), usecols=input_cols)
    
    df_daily = df_daily.apply(format_dates, axis=1)
    df_monthly = df_monthly.apply(format_dates, axis=1)
    
    lon = site_data['LOCATION_LONG'].values[0]
    lat = site_data['LOCATION_LAT'].values[0]
    igbp = site_data['IGBP'].values[0]
    
    df_daily['Lon'] = lon; df_daily['Lat'] = lat; df_daily['IGBP'] = igbp
    df_monthly['Lon'] = lon; df_monthly['Lat'] = lat; df_monthly['IGBP'] = igbp
    
    daily = daily.append(df_daily[['Lon', 'Lat', 'Year', 'DOY', 'IGBP']+benchmark_cols])
    monthly = monthly.append(df_monthly[['Lon', 'Lat', 'Year', 'Month', 'IGBP']+benchmark_cols])
    
    df_daily[['Year', 'MonthDay']+forcing_cols].to_csv(os.path.join(BASE_DIR, OUTPUT_DIR, '{}.csv'.format(site)),
                                                        sep='\t',
                                                        index=False, header=False,
                                                        float_format='%.3f')
    
daily.rename(columns=column_map, inplace=True)
monthly.rename(columns=column_map, inplace=True)

daily.to_csv(os.path.join(BASE_DIR, OUTPUT_DIR, 'daily.csv'), sep='\t', index=False)
monthly.to_csv(os.path.join(BASE_DIR, OUTPUT_DIR, 'monthly.csv'), sep='\t', index=False)
print('Finished!')
