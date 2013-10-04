### 
### load some useful tools/functions
#source("sql_tools.R")
library(RSQLite)

# connect to an existing database
db.conn <<- dbConnect("SQLite", dbname="e:/Daten/iLand/projects/AFJZ_experiment1203/output/bare_soil_afi413.sqlite" )

### load data from the iLand output table
stand <- dbReadTable(db.conn, "stand")
# alternatively (using sql_tools): stand <- query("select * from stand")

summary(stand)

## stacked bar chart, e.g. for basal area
library(ggplot2)

head(stand)
# aggregate to values for each species and year (i.e. averaging over all the resource units)
stand.avg <- aggregate(stand, list(year=stand$year, species=stand$species), FUN="mean")
head(stand.avg)

# now we can plot them:
ggplot(stand.avg, aes(x=year,y=basal_area_m2,group=species,fill=species)) + geom_area() + labs(xlab="year", ylab="basal area m2", title="average basal area")
## stem numbers
ggplot(stand.avg, aes(x=year,y=count_ha,group=species,fill=species)) + geom_area()
ggplot(stand.avg, aes(x=year,y=cohortCount_ha,group=species,fill=species)) + geom_area()

ggplot(stand.avg, aes(x=year,y=LAI,group=species,fill=species)) + geom_area()