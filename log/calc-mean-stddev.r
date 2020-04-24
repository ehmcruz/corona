#!/usr/bin/env Rscript

library(tidyverse)
library(maditr)
library(reshape)

argv <- commandArgs(TRUE)

if (length(argv) != 2) {
	stop(paste0(argv[1], " <directory> <csvout>"), call.=FALSE)
}

dirname = argv[1]
csvout = argv[2]

#csvout <- gsub("file_", "", csvout)

print(paste0("dirname ", dirname, " csvout ", csvout))
flist <- list.files(path = dirname, pattern = "/*.csv")

for (i in 1:length(flist)) {
	flist[i] = paste0(dirname, "/", flist[i])
}

print(flist)
#setwd("~/R/cruz")
#quit()
# leitura dos dados - busca todos *.csv e faz read_csv neles, salvando em um dataframe
raw_data <- flist %>% map_df(~read_csv(.)) %>% as.data.frame()

#print(raw_data)

#transformando de coluna para linha
df <- melt(raw_data, id="cycle")

#test <- df %>% group_by(cycle, variable)
ns <- length(flist)

tstudent <- qt(0.95, df=ns)

print(paste("number of samples", ns, "student table 95% interval", tstudent))
#stop()

# magica -- calculando media e desvio
df <- df %>% 
            group_by(cycle, variable) %>%
            summarize(mean=mean(value), se=tstudent*sd(value)/sqrt(n())) %>%
            as.data.frame()

#print(df)
#stop()

# gamb para dividir em 2 arquivos -- aqui para mean.out
dm <- df
dm$se <- NULL

# aqui para se.out
ds <- df
ds$mean <- NULL

#convertendo de linha para coluna (como era o original)
dk <- dcast(dm, cycle ~ variable)

#salvando
write_csv(dk, paste0(csvout, "-mean.csv"))

#convertendo de linha para coluna (como era o original)
dk <- dcast(ds, cycle ~ variable)

#salvando
write_csv(dk, paste0(csvout, "-se.csv"))
