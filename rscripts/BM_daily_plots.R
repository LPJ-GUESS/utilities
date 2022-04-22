rm(list=ls())
library(ggplot2)
library(dplyr)
library(reshape2)
library(gridExtra)



### SETTINGS
data_folder <- 'C:/Data/LPJ/sample_BMdaily' ##set folder to where dcflux.out and dlai.out are
lon <- 11.3175 # location of output
lat <- 47.1167 
year_range <- c(2000,2012)
days_filt <- 7 #number of days for average filter
lai_vars <- c('Date','C3G','C3G_pas','C4G','C4G_pas') #pfts to show in LAI plot



### Make plots

f21 <-rep(1/days_filt,days_filt) 


setwd(data_folder)



nee_plot <- read.table('dcflux.out',header=T) %>% 
  filter(Lon == lon & Lat == lat & Year>=year_range[1] & Year <= year_range[2]) %>% 
  mutate(Date=as.Date(paste(Year,substr(as.Date(Day, format = "%j", origin=paste("1.1.", Year)),5,10),sep=''))) %>% 
  mutate(value = stats::filter(NEE, f21, sides=2)) %>% 
  ggplot() + geom_line(aes(x=Date,y=value)) + ylab(expression(paste("NEE (kgC ",m^"-2",day^"-1",")"))) + theme_bw() +
  ggtitle(paste(days_filt,'days filt, lon =',lon,', lat =',lat))



lai_plot <- read.table('dlai.out',header=T) %>% 
  filter(Lon == lon & Lat == lat & Year>=year_range[1] & Year <= year_range[2]) %>% 
  mutate(Date=as.Date(paste(Year,substr(as.Date(Day, format = "%j", origin=paste("1.1.", Year)),5,10),sep=''))) %>% 
  dplyr::select(lai_vars) %>% reshape2::melt(., id.vars="Date") %>% 
  ggplot() + geom_line(aes(x=Date,y=value,colour=variable)) + ylab(expression(paste("LAI " ,m^"-2",m^"-2",")"))) + theme_bw() +
  ggtitle(paste('lon =',lon,', lat =',lat)) + theme(legend.position="bottom")



grid.arrange(nee_plot,lai_plot)




