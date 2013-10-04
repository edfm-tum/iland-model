### SQL helping function

# we use the R library RSQLite for all database accesses
# look up the help for the avaiable features.
library(RSQLite)

db.conn <- NA
query<-function(query_str) {
  res <- dbSendQuery(db.conn, query_str) 
  data <- fetch(res, n=-1)
  if (dbGetException(db.conn)$errorNum>0)
    print(dbGetException(db.conn)$errorMsg)
  dbClearResult(res)
  data
}

## species colors
# species_colors<-c("red", "blue", "green", "gray")
# names(species_colors)<-c("piab","fasy","lade", "pisy")
# species.color <- function(species) { 
#   if (species %in% names(species_colors)) {
#     return (species_colors[species]); 
#   } else {
#     return ("black");
#   }
# }

### query all tables from the output database
query("SELECT name FROM sqlite_master WHERE type='table'"); # use select * for more details!