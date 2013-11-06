### 
### load some useful tools/functions
#source("sql_tools.R")
library(RSQLite)

# connect to an existing database
db.conn <<- dbConnect("SQLite", dbname="e:/Daten/iLand/projects/Hainich/output/Hain_pnv.sqlite" )

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
cols.species <- c("abal"="#088A29",
                  "acca"="#F3F781",
                  "acpl"="#86B404", 
                  "acps"="#58FAD0", 
                  "algl"="#61210B", 
                  "alin"="#A4A4A4", 
                  "alvi"="#0B3B17", 
                  "bepe"="#2E64FE", 
                  "cabe"= "#F7BE81", 
                  "casa"="#A9F5A9", 
                  "coav"="#58D3F7",
                  "fasy"="#2EFE2E", 
                  "frex"="#FF0000", 
                  "lade"="#8A4B08", 
                  "piab"="#FFFF00", 
                  "pice"="#FF8000", 
                  "pini"="#610B21", 
                  "pisy"="#B18904", 
                  "poni"="#000000", 
                  "potr"="#610B5E", 
                  "psme"="#DF0174", 
                  "qupe"="#F6CED8", 
                  "qupu"="#FA5882", 
                  "quro"="#B40431", 
                  "rops"="#F781D8", 
                  "saca"="#F5ECCE", 
                  "soar"="#E6E0F8", 
                  "soau"="#B40404", 
                  "tico"="#9F81F7",
                  "tipl"="#8000FF", 
                  "ulgl"="#DF7401")

# color codes from species parameters (see section below)
cols.species <- c("abal"="#2F1BE0",
                  "lade"="#1BD3E0",
                  "pisy"="#F7430C",
                  "fasy"="#1BE06D",
                  "quro"="#E08E1B",
                  "acps"="#E01BD9",
                  "frex"="#871BE0",
                  "cabe"="#1B32E0",
                  "bepe"="#1B91E0",
                  "alin"="#1BE0AF",
                  "qupe"="#22E01B",
                  "psme"="#D6F288",
                  "algl"="#F2D288",
                  "casa"="#F29488",
                  "pini"="#F288A6",
                  "acca"="#DB88F2",
                  "acpl"="#9988F2",
                  "qupu"="#88C6F2",
                  "pice"="#88F2D4",
                  "soau"="#86E38C",
                  "soar"="#E3D586",
                  "coav"="#E3BB86",
                  "alvi"="#E39C86",
                  "potr"="#02B4FA",
                  "poni"="#8C70B5",
                  "tico"="#02E9FA",
                  "tipl"="#02FA6E",
                  "ulgl"="#82FA02",
                  "saca"="#FAFA02",
                  "rops"="#FA8602")

# now we can plot them:
ggplot(stand.avg, aes(x=year,y=basal_area_m2,group=species,fill=species)) + geom_area() + labs(xlab="year", ylab="basal area m2", title="average basal area") + scale_fill_manual(values=cols.species)
## stem numbers
ggplot(stand.avg, aes(x=year,y=count_ha,group=species,fill=species)) + geom_area()
ggplot(stand.avg, aes(x=year,y=cohortCount_ha,group=species,fill=species)) + geom_area()

ggplot(stand.avg, aes(x=year,y=LAI,group=species,fill=species)) + geom_area()


### e.g. LAI per resource unit in the first three years
par(mfrow=c(1,3))
for (y in c(1,2,3))
  boxplot(tapply(stand$LAI[stand$year == y], stand$ru[stand$year == y], sum), ylim=c(0,10), main=paste("Year", y))


### extract color codes from species parameter table ####
db.conn <<- dbConnect("SQLite", dbname="e:/Daten/iLand/projects/Hainich/database/param_2202.sqlite" )

### load data from the iLand output table
species.params <- dbReadTable(db.conn, "species")
s<-""
for (i in 1:dim(species.params)[1])
  s <- paste(s, '"',species.params$shortName[i], '"="#', species.params$displayColor[i], '",\n', sep="")

writeClipboard(s) ## copy to clipboard 
