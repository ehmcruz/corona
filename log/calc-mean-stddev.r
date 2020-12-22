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

#

# magica -- calculando media e desvio
df <- df %>%
            group_by(cycle, variable) %>%
            summarize(mean=mean(value), stddev=sd(value), min=min(value), max=max(value), se=tstudent*sd(value)/sqrt(n()), q10=quantile(value, probs=c(.1), na.rm = FALSE, names = FALSE), q50=quantile(value, probs=c(.5), na.rm = FALSE, names = FALSE), q90=quantile(value, probs=c(.9), na.rm = FALSE, names = FALSE) ) %>%
            as.data.frame()

#print(df)
#stop()

#head(df)
#quit()

# for debug
#df <- df %>% filter(variable == "ac_infected_state_ST_CRITICAL")

# gamb para dividir em 6 arquivos -- aqui para mean.out

d <- df %>% select(cycle, variable, mean)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-mean.csv"))

d <- df %>% select(cycle, variable, stddev)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-stddev.csv"))

d <- df %>% select(cycle, variable, min)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-min.csv"))

d <- df %>% select(cycle, variable, max)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-max.csv"))

d <- df %>% select(cycle, variable, se)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-se.csv"))

d <- df %>% select(cycle, variable, q10)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-q10.csv"))

d <- df %>% select(cycle, variable, q50)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-q50.csv"))

d <- df %>% select(cycle, variable, q90)
dk <- dcast(d, cycle ~ variable)
write_csv(dk, paste0(csvout, "-q90.csv"))

