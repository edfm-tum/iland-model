# BITE Examples

BITE examples is a collection of different biotic agents showcasing the various functionalities of BITE 
and how the agent parameters are setup in Javascript. Each agent has a special example of some individual BITE application:


Agent | example feature 
----------| ----------
Heterobasidion root rot | primary dispersal in landscape and secondary dispersal within cell
Gypsy moth | cyclic outbreaks
Roe deer | introduction everywhere in the landscape without dispersal kernel
Ash dieback | host tree resistance
Asian long-horned beetle | management interventions
Mastodon | creating impact on different tree size classes

## Heterobasidion root rot

Heterobasidion annosum is one of the most destructive forest pathogens in the northern hemisphere, causing root rot specifically on conifers (Garbelotto and Gonthier, 2013). Different species of the group are well established in many regions of Eurasia and North America, but it is currently also spreading into new areas (e.g. Bérubé et al., 2018). Heterobasidion sp. spread via spore infections through fresh stumps, e.g. created by tree harvesting. Within the stand the fungus spreads vegetatively via mycelia (Garbelotto and Gonthier, 2013). While Heterobasidion sp. spreads only over short distances (Kallio, 1970; Möykkynen et al., 1997) it can endure at a site for several tree generations (Stenlid and Redfern, 1998). 99.5% of the spores land within few hundred meters, with the remaining 0.5% responsible for the long distance dispersal of the pathogen (Kallio, 1970). In BITE, dispersal was simulated with a power law function (Kallio, 1970; Möykkynen et al., 1997), and the colonization was restricted to cells were fresh stump surfaces were available for spore germination (Rishbeth, 1951). Population dynamics were simulated based on a logistic growth model, with consumption and growth rate parameters derived from (Honkaniemi et al., 2017b). The impact on infested trees is simulated as a reduction in root biomass. 

Heterobasidion uses five different core modules of BITE: 1) Introduction, 2) Dispersal, 3) Colonization, 4) Population dynamics and 5) Impact. First, the details of root rot life cycle  are parametrized. Introduction and dispersal are simulated with BiteDispersal item, where the agent is introduced into the landscape before the first dispersal during the first simulation year with `onBeforeSpread` using a helpful `randomSpread` function. This function can be used to introduce the agent on random points at any given time during the simulation. 

Heterobasidion root rot is an example agent of two different dispersal methods. First, the primary dispersal in air by basidiospores is simuated with a simple power-law kernel with a `maxDistance` of 1000 meters. Colonization of cells is dependent here on forest management as harvesting creates stumps favoring the spore germination and colonization of a cell. Thus, we track how much fresh stump surface each cell contains. The secondary dispersal in cells where Heterobasidion have established is with mycelia via root contacts. This we have taken into account in this example code in the `BiteImpact`item where the trees are affected from the cell center outwards mimicking a typical expanding mortality/infection center for Heterobasidion root rot. The impact itself is a decrease of root biomass, which in iLand reduces the biomass of both coarse and fine roots affecting the carbon resources of a tree and further affecting the tree growth eventually leading to tree mortality.

```javascript
var Heterobasidion = new BiteAgent({
    name: "Heterobasidion root rot", description: "Root disease",
    cellSize: 20,
    climateVariables: ['MAT'], //mean annual temperature as climate variable for the BiteAgent items
	lifecycle: new BiteLifeCycle({ voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
				dieAfterDispersal: false, // cell does not die even with dispersal
				spreadFilter: 'yearsLiving>1', // condition that needs to be met before spread 
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1   // min. frequency of spread (1/years)
    }),

    dispersal: new BiteDispersal({
        kernel: 'min((x)^-1.5,1)', //simple power-law kernel modified from Kallio 1970, Möykkynen et al 1997  
        debugKernel: 'temp/kerneltest_heterobasidion.asc',  //raster file for debugging the kernel function
        maxDistance: 1000,   // maximum spreading distance is 1000m
        onBeforeSpread: function (bit) { if (Globals.year < 2) { randomSpread(1, bit.grid); console.log("added 1 px"); } }  //function to introduce the agent on first simulation year in a random cell (see below that coordinates are given for landscape centerpoint
    }),

    colonization: new BiteColonization({
        dispersalFilter: 'rnd(0,1) < (dispersalGrid)+0.005', // stochasticity included in the dispersal and colonization processes with random number generator, additional 0.5% chance for long distance dispersal (1-400 km) derived from Möykkynen et al 1997 and Kallio 1970
        cellFilter: 'stumpBasalArea>0',  //stumpBasalArea is a vegetation variable from iLand. If the cell is harvested at t, then the stumpBasalArea is x m2. If the time since harvest is more than 0, then stumpBasalArea=0. Heterobasidion spores need freshly cut stumpBasalArea for colonization.
        initialAgentBiomass: 0.01 // the initial agent biomass is assumed to be low as colonization can occur from few tiny spores
    }),

    growth: new BiteBiomass({
        hostTrees: 'species=pisy', // define the hosttrees to be Scots pine (Pinus sylvestris)
        hostBiomass: function (cell) {	//calculate the host biomass available for agent consumption
            if (cell.value('yearsLiving') < 10) {  //first, we assume that it takes roughly ten years for the agent to proceed from initial stump to infect healthy tree roots 
                var bm_stumps = cell.value('stumpRootMass'); // thus, calculate here only the root mass of stumps available for consumption
                return bm_stumps;   //total stump biomass
            } else {   //after 10 years since initial colonization, trees can be infected and consumed
                var bm_stumps = cell.value('stumpRootMass'); // rootmass of all pine stumps
                var bm_trees = cell.trees.sum('rootmass'); // rootmass of all pine trees
                var rootMass = bm_stumps + bm_trees; //total biomass for the fungi to consume
                return rootMass;
            }
        },
        mortality: 0,  // no additional mortality
        growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))', // logistic growth function, where K=hostbiomass / consumption; M=agentBiomass; r=relative growth rate coefficient; t=time
        growthRateFunction: '((6.52*MAT+(4.29*100-6.52*19))/100)',  //relative growth rate coefficient, we here assume a temperature related growth rate by Honkaniemi et al 2017
        consumption: '(6.52*MAT+(4.29*100-6.52*19))/100', // for simplicity, we assumed here the consumption to be equal to the agent growth rate
        growthIterations: 10 //10 iterative rounds of biomass calculation in case the biomass in the cell runs out during the time step
    }),

    impact: new BiteImpact({
        impactFilter: 'yearsLiving>10', //impact occurs once the agent has colonized the cell for more than 10 years (affects living trees)
        impact: [
		   { target: 'roots',  //target the root compartment of trees
		       maxBiomass: function (cell) { var agentimp = cell.value('agentImpact') - ((3.141592 * cell.value('stumpBasalArea') * 0.5) / 3); return agentimp; }, //agentImpact includes both stump and tree rootmass, but we want in this case to reduce only the tree rootmass from the tree list trees. Thus we need to approximate the stump rootmass value assuming that single stump was infected. We assume the stump root system as a cone.
		       fineRootFactor: 1,  //equal fraction of fine roots is affected compared to the consumed coarse roots
		       order: 'mod(x+10,20)^2 + mod(y+10,20)^2'  // trees are affected from the cell center in a circular towards the edges of the cell
		   }]
    }),

    output: new BiteOutput({
        outputFilter: "active=true",
        tableName: 'BiteTabx',
        columns: ['yearsLiving', 'MAT', 'stumpBasalArea']

    }),
    onYearEnd: function (agent) {
        agent.saveGrid('yearsLiving', 'temp/heterobasidion_yliv.asc');
        agent.saveGrid('cumYearsLiving', 'temp/heterobasidion_cyliv.asc');
        agent.saveGrid('active', 'temp/heterobasidion_active.asc');
        agent.saveGrid('stumpBasalArea', 'temp/heterobasidion_baharvest.asc');
        agent.updateVariable('stumpBasalArea', 0); // reset the grid
    },

    onTreeRemoved: function (cell, tree, reason) {
        if (reason == Tree.RemovedHarvest)
        //Bite.log('tree harvested:' + cell.info() + ":" + tree.species);
            var ba = cell.agent.exprBasalArea.value(tree);
			var rm = cell.agent.exprRootMass.value(tree);
			var sd = cell.agent.exprDensity.value(tree);
        cell.setValue('stumpBasalArea', cell.value('stumpBasalArea') + ba);
		cell.setValue('stumpRootMass', cell.value('stumpRootMass') + rm);
		cell.setValue('stumpDensity', cell.value('stumpDensity') + sd);
        //Bite.log('tree harvested:' + cell.info() + ":" + tree.species + ":" + ba);
    },

    onSetup: function (agent) {
        // called during setup of the agent
        agent.onTreeRemovedFilter = Tree.RemovedHarvest | Tree.RemovedDisturbance;
        // create a variable that is used for tracking tree harvests
        agent.addVariable('stumpBasalArea');
        agent.addVariable('stumpRootMass');
        agent.addVariable('stumpDensity');
        // add a TreeExpr for efficient access to tree variables
        agent.exprBasalArea = new TreeExpr("basalarea");
        agent.exprRootMass = new TreeExpr("rootmass");
        agent.exprDensity = new TreeExpr('1');
    }

});

function randomSpread(n, gr) {
	for (var i=0;i<n;++i) {
		var x = Math.random()*gr.width;
    var y = Math.random()*gr.height;
    gr.setValue(125,125,1);
	}
}
```
## Gypsy moth

European gypsy moth (from hereafter gypsy moth) is a defoliator native to Europe where it causes substantial disturbance especially on oaks (Mcmanus and Csóka, 2007). In 1869 it was also introduced to North America, where it became an invasive pest seriously threatening oak forests in the Northeastern USA (Elkinton and Liebhold, 1990). Adult gypsy moths are poor dispersers, but the first instar larvae spread passively over short distances via wind (Hunter and Elkinton, 2000). However, human-aided long distance dispersal is driving the invasion in North America (Liebhold et al., 1992). The development of a gypsy moth from egg to adult takes one season and during that development, each larvae consume about 3–4 g of foliage (Sharov and Colbert, 1996). Outbreaks of gypsy moth typically last for several years and occur both in Europe and North America in 8 – 12 year intervals (Johnson et al., 2005). Gypsy moth dispersal was approximated with a Gaussian dispersal kernel in BITE (Elderd et al., 2013), and its population dynamics was simulated with a logistic growth equation based on growth rates modified from Lustig et al. (2017). The simulated impact was consumption of foliage biomass, with preference for small over tall trees.   

Gypsy moth uses five different core modules of BITE: 1) Introduction, 2) Dispersal, 3) Colonization, 4) Population dynamics and 5) Impact. First, the details of gypsy moth life cycle  are parametrized. Gypsy moth serves as an example for an agent with periodic outbreaks. Here, the outbreak cyclicity is introduced arbitrarily with parameters, but could also be achieved with using specific functions in `BiteBiomass` describing the population dynamics. 

Introduction and dispersal are simulated with BiteDispersal item, where the agent is introduced into the landscape before the first dispersal during the first simulation year with `onBeforeSpread` using a helpful `randomSpread` function. This function can be used to introduce the agent on random points at any given time during the simulation. Dispersal here is defined to exclude human transport of gypsy moth and thus the `kernel` is simple Gaussian kernel with `maxDistance` of 50 meters. Gypsy moth can colonize a cell when the temperature sum (GDD10) for the cell is over 500 degree days and the host species, in this case Quercus robur is present. Population dynamics are simulated with the default logistic growth equation with modifications to increase the population growth rate during outbreaks and slow it down after. The vegetation impact is simulated start from the shortest trees and continue from tree to tree until the requirement for annual consumption of foliage is met. 

```javascript
var GypsyMoth = new BiteAgent({
	name: "Gypsy Moth", description: "defoliator", 
	cellSize: 10,
	climateVariables: ['GDD10'], 
	lifecycle: new BiteLifeCycle({ voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
				dieAfterDispersal: false, // cell does not die even with dispersal
				spreadFilter: 'yearsLiving>0', // condition that needs to be met before spread 
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1,   // min. frequency of spread (1/years)
				outbreakStart: 'rnd(8,12)', //random number of years for outbreak interval, calculated after each outbreak
				outbreakDuration: 'rnd(2,4)' //random number for outbreak duration, calculated after each outbreak
		}),
				
	dispersal: new BiteDispersal({
	kernel: '(1/3.141592*21.32637^2)*exp(-(x^2/21.32637^2))', //Gaussian dispersal kernel by Elderd et al (2013)
	debugKernel: 'temp/kerneltest_gypsymoth.asc',  //raster file for debugging the kernel function
	maxDistance: 50,  // maximum spreading distance is 50m
	onBeforeSpread: function(bit) {  if (Globals.year<2) {randomSpread(1, bit.grid); console.log("added 1 px");} }  //function to introduce the agent on first simulation year in a random cell (see below that coordinates are given for landscape centerpoint
		}), 

	colonization: new BiteColonization({ 
		dispersalFilter: 'rnd(0,1) < dispersalGrid',  // stochasticity included in the dispersal and colonization processes with random number generator
		cellFilter: 'GDD10>500',   //cell must meet the criteria of 500 degree days (base temperature 10C)
		treeFilter: 'species=quro', // cell must contain Common oak (Quercus robur) as host
		initialAgentBiomass: 250/100*0.0016 // initial agent biomass is calculated assuming 250 individual per ha are needed for succesful colonization (2.5 in 10m cell) and each moth weighs on average 1.6 grams
	   }),

	growth: new BiteBiomass({
		hostTrees: 'species=quro', // // define the hosttrees as above
		hostBiomass: function(cell) {   // calculate the potential biomass available in the cell for agent consumption
		   cell.reloadSaplings(); // trees are loaded automatically, but saplings need to be reloaded here
		   cell.saplings.filter('species=quro'); // define the sapling hosts
		   var bm_saplings = cell.saplings.sum('nrep*foliagemass'); // calculate the available total biomass for saplings = foliage biomass * number of represented stems per cohort
		   var bm_trees = cell.trees.sum('foliagemass'); // calculate the available total biomass for trees
		   return bm_saplings + bm_trees;   // total sum of host biomass in the cell
		},
		mortality: function(cell){   //external mortality is applied here, antagonists cause additional mortality driving the outbreaks down. Here, the outbreakYears is used to control the mortality
			var oy = cell.value('outbreakYears');  //enter the cell value outbreakYears
			if ( oy == 0) {
			return 0;			//if there is no ongoing outbreak, the additional mortality is 0
			} else {
			var death = (0.95/(1+Math.exp(-(oy-2))));  //during outbreaks, the probability for additional mortality (antagonists) increases exponentially
			return death;
			}
		}, 			
		growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))', // logistic growth function, where K=hostbiomass / consumption; M=agentBiomass; r=relative growth rate coefficient; t=time
		growthRateFunction: function(cell) {   //relative growth rate coefficient, this is assumed to exponentially increase when outbreak starts
			var oy = cell.value('outbreakYears'); //enter the cell value outbreakYears
			if ( oy == 0) {
			return 0;			//if there is no ongoing outbreak, the population is in equilibrium and r=0
			} else {
			var grate = (1.3/(1+Math.exp(-(oy-1))));  //during the outbreak, the growth rate is assumed to grow exponentially to maximum 1.3 fold 
			return grate;
			}
		},
		consumption:  (0.004*520*0.5*0.1)/0.0016, // host consumption by agent unit - assumption: total consumption  by single larvae survivng to adulthood is 4 grams of leaves, the number of eggs is 520 and the survival of larvae through instars is 0.5*0.1=0.05. Each adult is assumed to weigh 1.6 grams. 
		growthIterations: 1  //only one growthiteration
		}),		
		
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0',  //impact occurs once the agent has consumed the first biomass units of the host
		impact: [ 			//impact arrays 
		   {target: 'foliage', maxBiomass: 'agentImpact', order: "height"} // foliage of the host trees is consumed tree by tree from smallest to largest until the annual total consumption is reached
		]
	}),
	
	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving', 'hostBiomass', 'agentImpact','agentBiomass']
		
	}),
	onYearEnd: function(agent) { 
			agent.saveGrid('yearsLiving', 'temp/gypsymoth_yliv.asc');
		agent.saveGrid('cumYearsLiving', 'temp/gypsymoth_cyliv.asc');
		agent.saveGrid('active', 'temp/gypsymoth_active.asc'); }

});

function randomSpread(n, gr) {
	for (var i=0;i<n;++i) {
		var x = Math.random()*gr.width;
		var y = Math.random()*gr.height;
    gr.setValue(250,250,1);
	}
}
```

## Roe deer

Roe deer is a species of deer native to Europe. It is widespread throughout the whole continent from southern Europe to the Nordic countries, and is expanding its range due to warming climate and changes in land-use (Danilov et al., 2017; Valente et al., 2014). Roe deer is a relatively small deer species with an average body mass of ~20-30 kg (Andersen et al., 1998; Pettorelli et al., 2002). They are territorial animals and their habitat can range from agricultural landscapes to woodlands (Tixier and Duncan, 1996). Roe deer graze fresh grass, but a significant part of the diet consists also of sapling shoots (Tixier and Duncan, 1996). Silver fir (Abies alba) is one of the most favored tree species for browsing. In many areas browsing rates on the species are substantial, and can even lead to regeneration failure of Silver fir (Senn and Suter, 2003). In BITE, we assumed roe deer to populate the entire landscape with a constant density of 14 individuals per 100 ha (Senn and Suter, 2003) to be in line with the reference data for the impact analyses. The consumption was derived by combining daily diet preferences and consumption rates reported from different environments (Drozdz and Osiecki, 1973; Tixier and Duncan, 1996). The simulated impact was a loss of the leader shoot (and thus a loss of current year height growth) for saplings with a height of < 1.3 m.

Roe deer uses four different core modules of BITE: 1) Introduction, 2) Colonization, 3) Population dynamics and 4) Impact. Roe deer as a roaming browser is an example of an agent without a specific dispersal kernel and introduction to the entire landscape using `BiteDistribution` item. As the cell size of 100ha covers quite a variety of forest stands, we aggregate the tree information from the entire cell and check that the host with height <1.3m is present. The carrying capacity for the entire cell is calculated as a foliage mass of all suitable hosts and that limits the population dynamics of the agent. Impact is then simulated with the `browsing` impact target meaning that the growth of the sapling is 0 for the given year mimicking browsing of leader shoots. The share of affected trees is calculated as a ratio between consumed biomass and total available biomass.

```javascript
var roe_deer = new BiteAgent({
	name: "Roe deer", description: "Browser", 
	cellSize: 1000,
	lifecycle: new BiteLifeCycle({ voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
				dieAfterDispersal: false, // cell does not die even with dispersal
				spreadFilter: 'agentBiomass>0', // condition that needs to be met before spread 
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1   // min. frequency of spread (1/years)
		}),
				
	dispersal: new BiteDistribution({	}), // agent is distributed evenly across the landscape 
	
	colonization: new BiteColonization({ 
		dispersalFilter: 'dispersalGrid', // 100% chance of new colonization
		saplingFilter:'species=abal and height<1.3', // Silver fir saplings with height less than 1.3 m are hosts
		initialAgentBiomass: 14*24 // the initial agent biomass in each cell is calculated assuming the roe deer density to be 14 animals per 100 ha * average mass of 24 kg per animal  
	   }),

	
growth: new BiteBiomass({
		hostTrees: '(species=abal) and height<1.3', // these trees are the hosts
		hostBiomass: function(cell) {
		   cell.reloadSaplings(); // reload the sapling list
		   cell.saplings.filter('species=abal and height<1.3'); // filter saplings to meet the host criteria
		   var bm_saplings = cell.saplings.sum('nrep*foliagemass'); // calculate the available total biomass for saplings = foliage biomass * number of represented stems per cohort
		   return bm_saplings; // total sum of host biomass in the cell
	    },
		mortality: 0, //no additional mortality
		growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))', // logistic growth function, where K=hostbiomass / consumption; M=agentBiomass; r=relative growth rate coefficient; t=time
		growthRateFunction: '0',  // no population growth, assume that the local population is in equilibrium (birth=death,immigrate=emigrate)
		consumption: 3.66, // kg agent/kg host, consumption calculated assuming daily diet preferences and consumption rates from different environments (Drozdz and Osiecki, 1973; Tixier and Duncan, 1996)
		growthIterations: 10  //10 iterative rounds of biomass calculation in case the biomass in the cell runs out during the time step
		}),		
		
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0', //impact occurs once the agent has consumed the first biomass units of the host
		impact: [      				   //impact arrays 
		   {target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass'} // browsing effect for saplings, the fraction affected is calculated as a fraction of consumed host biomass from total biomass
		]
	}),
	
	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving', 'hostBiomass', 'agentImpact','agentBiomass']
		
	}),
	onYearEnd: function(agent) { agent.saveGrid('yearsLiving', 'temp/roedeer_base.asc'); }

});
```

## Ash dieback

Hymenoscyphus fraxineus is the causal agent of ash dieback, a non-native disease that has affected Europe’s forests over the past three decades (Kowalski and Holdenrieder, 2009; McKinney et al., 2014), and is currently threatening ash (primarily Faxinus excelsior L.) populations all over the continent (Pautasso et al., 2013). Environmental factors such as soil moisture and temperature as well as stand variables like stand age and density have been linked to the epidemiology of the fungus (Skovsgaard et al., 2017). In BITE, its spread dispersal was simulated with an inverse power law dispersal kernel (Grosdidier et al., 2018). As recent results show a decrease of the disease with decreasing host density (Bakys et al., 2013), we assumed that only cells with a host tree density of ≥100 stems ha-1 over more than 3 years were susceptible to ash dieback. As we could not gather enough information to build a reliable agent population dynamics module we omitted this aspect in simulation. We assumed that if the pathogen is present, it causes heavy defoliation (50-100% of foliage mass removed) for a maximum of 30% of the host trees of a cell (Timmermann et al., 2017). In addition, we assumed that 1% of the trees are resistant to the disease (Kjaer et al., 2012; Wohlmuth et al., 2018).

Ash dieback is an example of an agent with limited, yet increasing, amount of literature available for parametrization. Thus, it uses only four of the core modules: 1) Introduction, 2) Dispersal, 3) Colonization, and 4) Impact. Information on population dynamics is scarce, but developing the population dynamics could be added later as information accumulates. Inverse power law function as a dispersal `kernel`function for ash dieback was used here. The spores of the agent disperse quite far and thus the `maxDistance`was set to 1500 meters. Colonization occurs in cells with the host species without any specific tree size requirements. The ash dieback is an example for a one simple, but coarse way to introduce host resistance in BITE. In the `BiteImpact`item the `treeFilter`is used to exclude 1% of the trees from impacts in the cell using modulo operations. The impact is then caused on both trees and saplings in iLand, by either reducing the foliage or killing a specific share of saplings. 

```javascript
var ash_dieback = new BiteAgent({
	name: "Ash dieback - Hymenoscyphus fraxineus", description: "Fungal disease", 
	cellSize: 50, 
	lifecycle: new BiteLifeCycle({ voltinism: 1, // max 1 generation per year
				dieAfterDispersal: false, // cell does not die after dispersal
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1,   // min. frequency of spread (1/years)
				spreadFilter: 'yearsLiving>0', // only symptomatic cells spread
				mortality: function(cell) {     // agent and cell mortality is assumed to depend on tree density in the cell 
				var Ntrees = cell.trees.sum('1'); // number of trees
                var density = (Ntrees*10000)/2500; // density of trees in the cell
				var yrl = cell.value('yearsLiving'); //time since colonization
				if ( density < 100 & yrl>3) {  //if the density falls below 100 trees per ha after 3 years from colonization, the cell is no longer suitable for Ash dieback
				return true;
				} else {
				return false;
				}
				}
		}),
				
    dispersal: new BiteDispersal({
	  kernel: '(((3.30-1)*(3.30-2))/(2*3.141592*206.26)^2*(1+x/206.26)^(-3.30))*1000', //Inverse power law function to calculate the dispersal kernel from Grosdidier et al (2019)
	  debugKernel: 'temp/kerneltest_ash.asc',  //raster file for debugging the kernel function
	  maxDistance: 1500,		// maximum spreading distance is 1500m
	  onBeforeSpread: function(bit) { if (Globals.year<2) {randomSpread(1, bit.grid); console.log("added 1 px");} }   //function to introduce the agent on first simulation year in a random cell (see below that coordinates are given for landscape centerpoint
		}), 

	colonization: new BiteColonization({ 
		dispersalFilter: 'rnd(0,1) < dispersalGrid', // stochasticity included in the dispersal and colonization processes with random number generator
		treeFilter: 'species=frex',  //European ash trees are hosts
		saplingFilter: 'species=frex'	//European ash saplings are hosts
	   }),
	   
/*	 Defolation impact from Timmerman et al 2017
The logistic functions were fitted visually to the data. Only the heaviest defoliation class 50-100% was used in the impact arrays.
*/
  	impact: new BiteImpact({ 
		impactFilter: 'yearsLiving>1',  //impact occurs only after the first year since colonization
		impact: [     //impact arrays
		   {treeFilter: 'mod(id, 100)<>42', target: 'foliage' ,fractionPerTree: 'rnd(0.5,1)', 	fractionOfTrees: '0.3/(1+exp(-(yearsLiving-2.5)))',order='height'}, //1% of the population is considered resistant to the disease (using modulo operator in the treeFilter), impact is on foliage and the experssions are derived from Timmerman et al 2017 
		   {target:'sapling', fractionOfTrees='0.8/(1+exp(-(yearsLiving-6)))' ,order='dbh'} //kill saplings	based on Timmerman et al 2017 starting from smallest to largest	   
		]
	}),                  

 

	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving']
		
	}),
	onYearEnd: function(agent) { agent.saveGrid('yearsLiving', 'temp/ashdieback_yliv.asc');
								 agent.saveGrid('cumYearsLiving', 'temp/ashdieback_cyliv.asc');
								 agent.saveGrid('active', 'temp/ashdieback_active.asc'); }
	

});

function randomSpread(n, gr) {
	for (var i=0;i<n;++i) {
		var x = Math.random()*gr.width;
    var y = Math.random()*gr.height;
    gr.setValue(50,50,1);
	}
}

```

## Asian long-horned beetle

Asian long-horned beetle (ALB) is an insect species native to China and Korea, attacking the stems of multiple deciduous tree species. Its larvae consume the wood, which can eventually lead to tree mortality (Haack et al., 1997; Hérard et al., 2006). Global trade has resulted in the introduction of ALB to many areas outside its native range (Eyre and Haack, 2017), as the species effectively disperses in wood packaging material. In the USA, it has been estimated that ALB could potentially destroy ~30% of all urban trees, and cause a significant disturbance to forest ecosystems (Nowak et al., 2001).There are several different estimates of the potential impact of ALB varying from ~3-30% (Faccoli and Gatto, 2016; Nowak et al., 2001), often based on the large-scale mortality of poplar plantations in the native habitat of the beetle in China (see Hu et al., 2009 and the references therein). However, quick eradication measures at infested sites have prevented accumulation of data to quantify real impacts caused by ALB. Dodds and Orwig (2011) studied the only large-scale infestation outside the native range in non-urban environment in Massachusetts, USA and found that tree mortality and growth losses were extremely rare even after more than 5 years after the infestations. ALB is a moderate disperser, and we here used a general leptokurtic dispersal kernel to simulate its spread (Shatz et al., 2016). We assumed that to colonize a cell, host with dbh>7.5 cm was needed to be present (Dodds and Orwig, 2011). Even though the life cycle of the agent is generally well-known (Haack et al., 2009), we didn’t find enough reliable quantitative information to parameterize the detailed agent population dynamics module of BITE. Furthermore, mortality rates remain uncertain; we here assumed that all trees would die 4 years after an infestation (Nowak et al., 2001)the annual mortality rates would be rather low (Dodds and Orwig, 2011) and we used a linearly increasing mortality rate from 0 to 2% over 10 year period to simulate the slow mortality process caused by the beetle. .

Asian long-horned beetle is using 4 of the core modules similar to ash dieback: 1) Introduction, 2) Dispersal, 3) Colonization, and 4) Impact. However, the information for asian long-horned beetle was more limited compared to ash dieback and many of the parameters were estimated based on more qualitative analysis from few publications. The dispersal kernels are often the most well-known parameters for BITE and that was the case for ALB as well, where several papers have been published estimating the agent disperal based on different data sources. The impact of ALB infestation was the least known parameter as in most cases when ALB are introduced the rapid eradication measures take place leading to increased mortality of trees, not due to agent itself but eradication harvests. Thus, ALB serves here as an example how management interventions could be applied in the BITE/iLand framework. When the `BiteImpact` is enabled, the first 10 years, ALB is assumed to kill annually 0.2% of the trees with >7.5cm diameter at breast height in the infested cells starting from the smallest trees. In addition, we assume here that it takes on average 3 years to detect an infestation and to start eradication. This is done here with the aid of iLands agent based management component allowing an intervention in the management program.

```javascript
var BarkBeetle = new BiteAgent({
	name: 'Asian longhorned beetle - Anolophora glabripennis', description: "Stem boring insect", 
	cellSize: 10, 
	lifecycle: new BiteLifeCycle({ voltinism: '1', // generations per year
				dieAfterDispersal: false, //function(cell){if (cell.value('yearsLiving')>4) {return 'true'}; return 'false';}, // after dispersal a cell dies
				spreadFilter: 'yearsLiving>0', // only symptomatic cells spread 
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1,   // min. frequency of spread (1/years)
				mortality: function(cell) {     // agent and cell mortality is assumed to depend on tree density in the cell 
				var Ntrees = cell.trees.sum('1'); // number of trees
				var yrl = cell.value('yearsLiving'); //time since colonization
				if ( Ntrees < 1 & yrl>2){		//density below 1 and time since colonization over 2 years
				return true;
				} else {
				return false;
				}
				}  
		}),
	
	dispersal: new BiteDispersal({
	kernel: 'exp(-0.1478*sqrt(x))', //dispersal kernel from Smith et al 2001
	debugKernel: 'temp/kerneltest_alb.asc', //raster file for debugging the kernel function
	maxDistance: 1000,  // maximum spreading distance is 1000m
	onBeforeSpread: function(bit) { if (Globals.year<2) {randomSpread(1, bit.grid); console.log("added 1 px");} }   //function to introduce the agent on first simulation year in a random cell (see below that coordinates are given for landscape centerpoint
		}), 
	
	colonization: new BiteColonization({ 
		dispersalFilter: 'rnd(0,1) < dispersalGrid', // stochasticity included in the dispersal and colonization processes with random number generator
		treeFilter: 'species=acps AND dbh>=7.5' // the cell must have Sycamore (Acer pseudoplatanus) as host and tree diameter needs to be over 7.5 cm (Dodds and Orwig 2011)
	   }),

	
	impact: new BiteImpact({ 
		impact: [			//impact arrays
		   {treeFilter: 'dbh>=7.5',target: 'tree', fractionOfTrees: 'min(yearsLiving*0.002,0.02)', order='-dbh'} // remove linearly 0.2% of trees in the cell every year until 10 years after which the mortality is constant
		   ],
		   onImpact: function(cell) { 
		    console.log('Impact on stand: ' + cell.value('standId'));
			ABELink(cell.value('standId'), cell.value('yearsLiving'), 3 ); // accumulate impact 			
		}
	}),
	
	
	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving']
		
	}),
	onSetup: function(agent) { 	//loads the standIds to track in which stand the BITE cell is 
		agent.addVariable('tfirst');
		var grid = Factory.newGrid();
		grid.load('gis/bite.stands.asc');
		agent.addVariable(grid, 'standId');
		}, // part of the Item
		
	onYearEnd: function(agent) { 
		agent.updateVariable('tfirst', function(cell) {
			if (cell.value('tfirst')>0) return cell.value('tfirst')+1; // increment
			if (cell.cumYearsLiving==1) return 1; // start
			return 0; 
		});
		agent.saveGrid('yearsLiving', 'temp/alb_yliv.asc');
		agent.saveGrid('cumYearsLiving', 'temp/alb_cyliv.asc');
		agent.saveGrid('tfirst', 'temp/alb_tfirst.asc');
		agent.saveGrid('active', 'temp/alb_active.asc'); }

});

function randomSpread(n, gr) {
	for (var i=0;i<n;++i) {
		var x = Math.random()*gr.width;
    var y = Math.random()*gr.height;
    gr.setValue(250,250,1);
	}
}


// Management Link (to ABE)
var stand_damage = {}

// accumulate damage for stand 'standId'
function ABELink(standId, damage,threshold) {
   if (!(standId in stand_damage))
      stand_damage[standId] = 0;
   stand_damage[standId] += damage;   

   if (stand_damage[standId] > threshold) {
	   console.log("accumulated damage for stand " + standId + " >1: " + stand_damage[standId] + "; starting management");
	   // invoke management for the stand
	   fmengine.runActivity(standId, 'disturbance_response');

	   stand_damage[standId] = 0; // reset damage counter again
   }   

}


```

In the agent-based management module of iLand (http://iland.boku.ac.at/ABE?highlight=abe) we used the following JavaScript description to cut all the trees in a stand affected by the biotic agent and exceeding the threshold given above:

``` 
var a_disturbance_response = { type: 'general',
                     schedule: 1000, // not reached 
                     action: function() {
						 		trees.loadAll();  
								trees.harvest();
								trees.removeMarkedTrees();
						 // do whatever needs to be done here
						 console.log('ABE: disturbance response');
					 }
}
``` 
## Mastodon

Mastodons were large mammals distantly related to elephants, inhabiting the forests of North America and Eurasia until their extinction ~10–11,000 years ago. Compared to mammoths (Mammuthus sp.), which were grazers, mastodons were forest-dwelling browsers with Picea spp. forming a significant part of their diet (Birks et al., 2018; Teale and Miller, 2012). Their estimated body mass was ~8000 kg, mastodons were thus slightly heavier than modern elephants although their shoulder height was roughly comparable (Larramendi, 2015). We assumed mastodons to inhabit the whole test landscape with an initial density of 1.5 individuals per 100 ha, corresponding to the estimated densities of Pleistocene megaherbivores (120 kg ha-1) (Bakker et al., 2016). Mastodon population growth rate was assumed to be 1% yr-1 using a logistic growth model. We assumed that mastodons were able to browse trees up to 4 m height, with a preference for trees between 0-2 m (Guy, 1976) and the occasional uprooting of trees, similar to modern elephants (Scheiter and Higgins, 2012; Shannon et al., 2008). The diet was assumed to consist of 20% Norway spruce (Picea abies (L.) Karst.). 

Mastodon uses four different core modules of BITE: 1) Introduction, 2) Colonization, 3) Population dynamics and 4) Impact. Mastodons, together with the roe deer, are examples of an agent without a specific dispersal kernel and introduction to the entire landscape using `BiteDistribution` item. See above the roe deer example for details on that. The mastodon is here as an example of varying the agent impacts on vegetation depending on tree size. The following code and examples on alternative impacts provides tips to cause impact on different tree size classes.

```javascript
var mastodon = new BiteAgent({
	name: "Mastodon", description: "Extinct browser", 
	cellSize: 1000,  //maximum reasonable cellsize regarding vegetation aggregation, mammal migration under development
	lifecycle: new BiteLifeCycle({ voltinism: 1, // max 1 generation per year, growth rate adjusts the true population growth
				dieAfterDispersal: false, // cell does not die even with dispersal
				spreadFilter: 'agentBiomass>0', // condition that needs to be met before spread 
				spreadDelay: 1, // number of years after colonization
				spreadInterval: 1   // min. frequency of spread (1/years)
		}),
				
	dispersal: new BiteDistribution({	}), // agent is distributed evenly across the landscape 
	
	colonization: new BiteColonization({ 
	    dispersalFilter: 'dispersalGrid', // 100% chance of new colonization
		treeFilter:'species=piab and dbh<=15', //Norway spruce with diameter at breast height less than 15cm are hosts
		saplingFilter:'species=piab', //all Norway spruce saplings are hosts
		initialAgentBiomass: 12000 // initial agent biomass is calculated assuming 1.5 mastodons per 100 ha with an average mass of 8tn per individual
	   }),

growth: new BiteBiomass({
		hostTrees: '(species=piab and dbh<=15)', // define the hosttrees as above
		hostBiomass: function(cell) {			// calculate the potential biomass available in the cell for agent consumption
		   cell.reloadSaplings();  // trees are loaded automatically, but saplings need to be reloaded here
		   cell.saplings.filter('species=piab'); // define the sapling hosts
		   var bm_saplings = cell.saplings.sum('nrep*foliagemass'); // calculate the available total biomass for saplings = foliage biomass * number of represented stems per cohort
		   var bm_trees = cell.trees.sum('foliagemass'); // calculate the available total biomass for trees
		   return bm_saplings + bm_trees;   // total sum of host biomass in the cell
		},
		mortality: 0,  //no additional mortality
		growthFunction: 'K / (1 + ( (K - M) / M)*exp(-r*t))', // logistic growth function, where K=hostbiomass / consumption; M=agentBiomass; r=relative growth rate coefficient; t=time
		growthRateFunction: '0.01',  //relative growth rate coefficient, the population growth rate is 1%
		consumption:  '(0.05*8000*365*0.2)/8000', // host consumption by agent unit - assumption: daily consumption is 5% of the body mass, 365 days, 80% of the diet is host plant
		growthIterations: 10  //10 iterative rounds of biomass calculation in case the biomass in the cell runs out during the time step
		}),		
		
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0',   //impact occurs once the agent has consumed the first biomass units of the host
		impact: [						//impact arrays 
		   {target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass', order:'height'}, // browsing effect for sapligns, the fraction affected is calculated as a fraction of consumed host biomass from total biomass
		   {treeFilter: 'dbh<=15', target: 'tree', fractionOfTrees: '0.05'} //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		]
	}),
	
	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving', 'hostBiomass', 'agentImpact','agentBiomass']
		
	}),
	onYearEnd: function(agent) { agent.saveGrid('yearsLiving', 'temp/mastodon_base.asc'); }

});
```

Alternative Impact 1. Here, the impact on fraction of trees is varying with the tree diameter. The smallest trees are affected the least (2.5% affected a year) while 10-15cm trees are killed the most, 7.5%. This is an easy way to incorporate explicit effects on forest structure.
```javascript
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0',   //impact occurs once the agent has consumed the first biomass units of the host
		impact: [						//impact arrays 
		   {treeFilter: 'dbh<=5', target: 'tree', fractionOfTrees: '0.025'}, //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		   {treeFilter: 'dbh>5 and dbh<=10', target: 'tree', fractionOfTrees: '0.05'}, 
		   {treeFilter: 'dbh>10 and dbh<=15', target: 'tree', fractionOfTrees: '0.075'} 

		]
	}),
```
Alternative Impact 2. Here, the fractions are the same, but instead of diameter, we use the tree heigth as a classifying variable. 
```javascript
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0',   //impact occurs once the agent has consumed the first biomass units of the host
		impact: [						//impact arrays 
//		   {target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass', order:'height'}, // browsing effect for sapligns, the fraction affected is calculated as a fraction of consumed host biomass from total biomass
		   {treeFilter: 'height<=8', target: 'tree', fractionOfTrees: '0.025'}, //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		   {treeFilter: 'height>8 and height<=12', target: 'tree', fractionOfTrees: '0.05'}, //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		   {treeFilter: 'height>12 and height<=15', target: 'tree', fractionOfTrees: '0.075'} //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)

		]
	}),
	
```
Alternative Impact 3. And here, the diameter classes and fractions have been changed. Notice, that here trees with 8-12 cm diameter do not get killed at all!
```javascript
	impact: new BiteImpact({ 
		impactFilter: 'agentImpact>0',   //impact occurs once the agent has consumed the first biomass units of the host
		impact: [						//impact arrays 
//		   {target: 'browsing', fractionOfTrees: 'agentImpact/hostBiomass', order:'height'}, // browsing effect for sapligns, the fraction affected is calculated as a fraction of consumed host biomass from total biomass
//		   {treeFilter: 'dbh<=5', target: 'tree', fractionOfTrees: '0.025'}, //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		   {treeFilter: 'dbh>5 AND dbh<=8', target: 'tree', fractionOfTrees: '0.05'}, //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)
		   {treeFilter: 'dbh>12 AND dbh<=15', target: 'tree', fractionOfTrees: '0.05'} //we assume that the mastodons were uprooting small diameter trees for forage in a similar way as their modern counterparts, elephants, in Africa do (Shannon et al 2008 see Scheiter & Higgins 2012)

		]
	}),
```
