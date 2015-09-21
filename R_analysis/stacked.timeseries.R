### 
### load some useful tools/functions
#source("sql_tools.R")
library(RSQLite)

# connect to an existing database
db.conn <<- dbConnect(RSQLite::SQLite(), dbname="e:/Daten/iLand/projects/HJA_WS12_v2/output/HJA_WS12.sqlite" )

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

#### provide fixed colors for species (in form of key-value pairs; the key is the species code, value is the color)
cols.species <- c("Tshe"="#101010", 
                  "Abam"="#202020", 
                  "Alru" =  "#303030", 
                  "Thpl" = "#404040", 
                  "Psme" = "#505050", 
                  "Acma" = "#606060", 
                  "var2" = "#707070", 
                  "var3" = "#808080")

# now we can plot them:
ggplot(stand.avg, aes(x=year,y=basal_area_m2,group=species,fill=species)) + geom_area() + labs(xlab="year", ylab="basal area m2", title="average basal area") + scale_fill_manual(values=cols.species)
## stem numbers
ggplot(stand.avg, aes(x=year,y=count_ha,group=species,fill=species)) + geom_area()
ggplot(stand.avg, aes(x=year,y=cohortCount_ha,group=species,fill=species)) + geom_area()

ggplot(stand.avg, aes(x=year,y=LAI,group=species,fill=species)) + geom_area()

cols <- rainbow(nrow(mtcars))
mtcars$car <- rownames(mtcars)
ggplot(mtcars, aes(mpg, disp, colour = car)) + geom_point() +
  scale_colour_manual(limits = mtcars$car, values = cols) +
  guides(colour = guide_legend(ncol = 3))


### e.g. LAI per resource unit in the first three years
par(mfrow=c(1,3))
for (y in c(1,2,3))
  boxplot(tapply(stand$LAI[stand$year == y], stand$ru[stand$year == y], sum), ylim=c(0,10), main=paste("Year", y))
