################################################################
### Create tables in the climate database for iLand using R ####
################################################################

# we use the R library RSQLite for all database accesses
# look up the help for the avaiable features.
library(RSQLite)
#

# connect to a existing or create a new database
db.conn <<- dbConnect("SQLite", dbname="e:/Daten/iLand/projects/AFJZ_experiment/database/new_database.sqlite" )

## set up a data frame of climate data using right columns and units
# see http://iland.boku.ac.at/climatedata for the required columns

# here load a test data set:
test.data <- read.delim("e:/Daten/BOKU/CCTame/climate/daily/ID014369.csv",sep=";")
head(test.data)
summary(test.data)

# set up the data frame
iland.climate <- data.frame(year=test.data$year,
                            month=test.data$month,
                            day=test.data$day,
                            min_temp=test.data$tmin,
                            max_temp=test.data$tmax,
                            prec=test.data$prec,
                            rad=test.data$rad,
                            vpd=test.data$vpd)
summary(iland.climate)


### save into the database: ####
## the table name is just an example. 
## However, this name is referred to in the project file.
dbWriteTable(db.conn, "climate014369",iland.climate, row.names=F)

# helpful, maybe:
# remove the table again:
dbRemoveTable(db.conn, "climate014369")

## check if it worked:
cmp <- dbReadTable(db.conn, "climate014369")
summary(cmp)