# iLand viewer R/Shiny part

LandscapeUI <- function(id) {
   ns <- NS(id)
    span(
     selectInput(ns("variable"), label = "Select variable",
                                choices = list("Basal area"="basal_area_m2", "Volume"="volume_m3")),
     actionButton(ns("refresh"), "Refresh...")
    )
   
                  

  
}

landscapePlotUI <- function(id) {
  ns <- NS(id)
  plotOutput(ns("plot"), width=700, height = 650)
}

Landscape <- function(input, output, session) {
  
  lscp <- data.frame()
  observeEvent(input$refresh, {
    cat("Refresh...")
    conn<-dbConnect(RSQLite::SQLite(), dbname = iland.output.path)
    lscp<<-dbReadTable(conn,"landscape")
    dbDisconnect(conn)
    cat(paste(dim(lscp)[1], "rows."))
    
  })
  
  # create plot
  output$plot <- renderPlot({
    cat(input$variable)
    if (input$refresh>0) {
      ggplot(lscp, aes(x=year,y=lscp[,input$variable],group=species,fill=species)) + 
        geom_area() + labs(x="year", y=as.name(input$variable)) + 
        scale_fill_manual(values=cols.species)
      
    }
  })
}