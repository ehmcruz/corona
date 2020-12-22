#!/usr/bin/env Rscript

args = commandArgs(trailingOnly=TRUE)

if (length(args) != 3)
	stop("Usage: plot-cycles.R <input.csv> <output.csv> <group_size>\n")

csv_input = args[1]
csv_output = args[2]
group_size = as.integer(args[3])

data = read.csv(csv_input, header = TRUE)

#print(data)

col_reproductive <- numeric(nrow(data))
col_group_infected <- numeric(nrow(data))

#print(nrow(data))
#print(col_group_infected)
#print(group_size)

for (i in 1:length(col_reproductive)) {
	ini = i - group_size + 1
	if (ini < 1)
		ini <- 1
	iniref = ini - 1

	col_group_infected[i] <- 0

	for (j in ini:i) {
		col_group_infected[i] <- col_group_infected[i] + data[j, "state_ST_INFECTED"]

		if (iniref < 1)
			col_reproductive[i] <- 0
		else if (col_group_infected[iniref] == 0)
			col_reproductive[i] <- 0
		else
			col_reproductive[i] <- col_group_infected[i] / col_group_infected[iniref]
	}
}

#print(col_reproductive)

data$group_infected <- col_group_infected
data$reproductive <- col_reproductive

data$g_reported = data$ac_infected_state_ST_MILD + data$ac_infected_state_ST_SEVERE + data$ac_infected_state_ST_CRITICAL

#data_input$test <- data_input$state_ST_INFECTED

#print(data)

write.csv(data, csv_output)
