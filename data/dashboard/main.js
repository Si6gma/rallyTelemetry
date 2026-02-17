//Global constants

//DOM elements
const parameters = document.querySelector("#parameters");
const simpleChart = document.querySelector("#simpleChart");
const display = document.querySelector('#DisplayOptions');
const lightdarkemoji = document.querySelector('#lightdarkemoji')
const addParameter = document.querySelector('#calcModal')

//Backend variables
var displaymode = display.value;
var csvAsJson = {};
var t = {};
var yParameters = {}
var datalist = [];
var parameterlist = [];
var myChart;
var prefersDarkScheme = window.matchMedia("(prefers-color-scheme: dark)").matches;  //lookup light dark settings of windows.
lightdarkemoji.textContent = prefersDarkScheme ? '🌙': '☀️';    //Set emoji of light dark mode to the one of windows setting

//function that changes dark light mode
function changelightdark() {
prefersDarkScheme = prefersDarkScheme ? 0:1;
    document.documentElement.style.setProperty('color-scheme', prefersDarkScheme ? "dark":"light");
    lightdarkemoji.textContent = prefersDarkScheme ? "🌙":"☀️";
    displaygraph(t['time [s]'], datalist, parameterlist);
}

lightdarkemoji.addEventListener("click", changelightdark); //event listener for dark light mode when is clicked on emoji.

//event lister for dark light mode for when windows setting is change. 
window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", () => {
    if (window.matchMedia("(prefers-color-scheme: dark)").matches != prefersDarkScheme) {
        changelightdark();
    }
});

//add eventlisteners for all mathOperators that are used in calcUI
const mathOperators = document.querySelectorAll('.mathOperator').forEach(mathOperator => {
    mathOperator.addEventListener('click', () => {
        document.getElementById('newParameterEquation').value +=  mathOperator.textContent
    })
})


//eventlistener for when display mode is changed. 
display.addEventListener("change", async function() {
    displaymode = display.value;
    displaygraph(t['time [s]'], datalist, parameterlist);
})

//main program
document.getElementById("csvFile").addEventListener("change", async function(event) {
    const file = event.target.files[0];
    csvAsJson = await readCsv(file);
    await getTime();   
    displayParameters();
    displayCreateParameterButton();
});

//Read csv
async function readCsv(file)   {
    return new Promise((resolve, reject) => {
        if (!file) return;
        
        const reader = new FileReader();
        const Json = {}
        reader.onload = async function(e) {
            const text = e.target.result;
            const lines = text.split("\n").map(line => line.trim()).filter(line => line); // Remove empty lines
            if (lines.length < 2) return; // Ensure we have at least a header + one row

            const headers = lines[0].split("\t").map(header => header.trim());
            

            // Initialize keys with empty arrays
            headers.forEach(header => {
                Json[header] = [];
            });

            // Process each row
            for (let i = 1; i < lines.length; i++) {
                const values = lines[i].split("\t").map(value => value.trim());
                
                // Add each value to its corresponding key
                headers.forEach((header, index) => {
                    Json[header].push(Math.round(parseFloat(values[index])*100)/100); 
                });
            }
            resolve(Json)
        };
        reader.onerror = (error) => reject(error);
        reader.readAsText(file);
    });
}

//seperatate t from y parameters
async function getTime()   {
    t = {'time [s]': []};
    t['time [s]'].push(csvAsJson['Log Timestamp [498]'][0]);
    for (let i = 1; i < csvAsJson['Log Timestamp [498]'].length; i++)  {
        new_t = Math.round((t['time [s]'][i-1] + csvAsJson['Log Timestamp [498]'][i]/1000)*100)/100;
        t['time [s]'].push(new_t);
    } 
    yParameters = {...csvAsJson};
    delete yParameters['Log Timestamp [498]'];
    }

    //display csv parameters
    function displayParameters() {
    //delete all old parameters
    const oldParameters = document.querySelectorAll(".parameter");
    oldParameters.forEach((oldParamter) => {
        oldParamter.remove();
    });


    //create new parameters
    if (JSON.stringify(yParameters) === '{}') {console.log('DisplayParamters returned'); return;} // check if jsonobject is empty. 

    for (const key in yParameters) {
        //for each key(parameter in the json object create a div with parameter class)
        const new_parameter = document.createElement("div");
        new_parameter.classList.add('parameter');


        //for each div with parameter class create a parameter label with the key as text and parameter_value as class. 
        const new_parameter_value = document.createElement("p");
        new_parameter_value.textContent = key;
        new_parameter_value.classList.add('parameter_value');
        new_parameter.appendChild(new_parameter_value);

        //for each div with parameter class add a checkbox with class parameter_checkbox.
        const new_parameter_checkbox = document.createElement("input");
        new_parameter_checkbox.setAttribute("type", "checkbox");
        new_parameter_checkbox.setAttribute("value", key);
        new_parameter_checkbox.setAttribute("name", "y-variable");
        new_parameter_checkbox.classList.add('parameter_check');
        new_parameter.appendChild(new_parameter_checkbox);
        parameters.appendChild(new_parameter);
    }
    parameterCheck = document.querySelectorAll(".parameter_check");
    //Eventlister for buttons
    parameterCheck.forEach((parameterSelected) => {
        parameterSelected.addEventListener("change", async function(event) {
            datalist = [];
            parameterlist = [];
            parameterCheck.forEach((checkbox) => {
            
                if (checkbox.checked)  {
                    datalist.push(yParameters[checkbox.getAttribute("value")]);
                    parameterlist.push(checkbox.getAttribute("value"));
                    
                }

            })
            displaygraph(t['time [s]'], datalist, parameterlist);
        })
    })
}

//display createParameterButton
function displayCreateParameterButton()   {
    if (document.querySelector('#createParameterButton') !== null) {
        return
    } else {        
        const createParameterButtonBox = document.querySelector("#createParameterButtonBox")
        const createParameterButton = document.createElement("button");
        createParameterButton.textContent = 'new parameter';
        createParameterButton.setAttribute('id', "createParameterButton");
        createParameterButtonBox.appendChild(createParameterButton);
        createCreateParameterEventListener();
}
}


//create eventlistener for create parameter
function createCreateParameterEventListener()    {
    document.getElementById('createParameterButton').addEventListener("click", () => {
        var newParameterName = document.getElementById('newParameterName').value = '';
        var equation = document.getElementById('newParameterEquation').value = '';
        const parameterbox = document.getElementById('calcUIleftBox')
        //delete all old parameters
        const oldcalcUIParameters = document.querySelectorAll(".calcUIparameter");
        oldcalcUIParameters.forEach((oldcalcUIParameters) => {
            oldcalcUIParameters.remove();
        });
        for (let key in yParameters) {
            const new_parameter = document.createElement("p");
            new_parameter.textContent = key;
            new_parameter.classList.add('calcUIparameter')
            parameterbox.appendChild(new_parameter)
            new_parameter.addEventListener('click', () => {
                document.getElementById('newParameterEquation').value +=  key
            })
        }
        addParameter.style.display = 'flex'; // show calcUI
        //eventlistener for submit button
        document.getElementById('submitNewParameter').addEventListener("click", createNewParameter)
    })        
}

//function that calculates new parameter
function createNewParameter() {
    //get new parameter name and equation.
    newParameterName = document.getElementById('newParameterName').value;
    equation = document.getElementById('newParameterEquation').value;
    //check if new parameter name and equation have values
    if (newParameterName === null || newParameterName === ''|| equation === null || equation === '') {
        alert("No input given");
        addParameter.style.display = 'none';
        document.getElementById('submitNewParameter').removeEventListener('click', createNewParameter)
        return;
    }else if (newParameterName in yParameters){             //check if name of new parameter is not already used.
        alert("Parameter name already exist");
        return;
    } else {
        // if name and equation check out try to calculate new parameter
        try {
            const result = [];
            for (var i = 0; i < t['time [s]'].length; i++) {
                const scope = {};
                let equation_t = equation
                let j = 0;
                for (let key in yParameters) {
                    if (equation.includes(key.toString())) {
                        j++
                        equation_t = equation_t.replace(new RegExp(escapeRegex(key), 'g'), `var${j}_${i}`)
                        Object.assign(scope, {[`var${j}_${i}`]: yParameters[key][i]});
                    }                
                }
                result.push(math.evaluate(equation_t, scope));
            }
            //check if new parameter has strange numbers like inf or NAN and give a warning. The new parameter is still added and can be used.
            const hasInvalid = result.some(n => !Number.isFinite(n));
            if (hasInvalid) {
                alert("New parameter contains values that can't be displayed by the graph like inf or NAN. Check for possibility of divide by 0 or if datasets are used that didn't contain numbers.");
            }
            //ad new parameter to yParameter object
            Object.assign(yParameters, {[newParameterName]: result})
            //refresh parameterlist
            displayParameters()
        } catch (error) {
            console.error("Invalid input:", error.message);
            alert("Error: Invalid mathematical expression! Please try again.");
            addParameter.style.display = 'none';
            return;
        }
        
    }
    // hide calcUI
    addParameter.style.display = 'none';
}
//create myChart object
function displaygraph(tValues, yValues, parameter) {
    let dsets = [];
    let ylimits = {};
    if (!prefersDarkScheme) {
        var gridColor = 'rgb(180, 180, 180)';
        var textcolor = 'rgb(0, 0, 0)';
    } else {
        var gridColor = 'rgb(70, 70, 70)';
        var textcolor = 'rgb(250, 250, 250)';
    }

    if (myChart) {
        myChart.destroy();
        myChart = null; // Clear reference
    }

    let multiplescales = {
        'x': {
            title: {
                display: true,
                text: 'Time (s)',
                color: textcolor
            },
            grid: {
            color: gridColor,
            },
            ticks: {
            color: textcolor
            }       
        }
    }


    for (i=0; i < yValues.length; i++) {
        dsets[i] = {
            label: parameter[i],
            data: yValues[i],
            borderWidth: 2,
            fill: false,
            yAxisID: `y${i}`,
            pointStyle: 'circle',
            pointRadius: 1,
            pointHoverRadius: 6, 
        };
        switch(displaymode)    {
            case "fully stacked":
                multiplescales[`y${i}`] = {                        
                    title: {
                        display: true,
                        text: parameter[i],   
                        color: textcolor 
                    },
                    
                    type: 'linear',
                    offset: true,
                    position: 'left',                        
                    stack: 'demo',
                    grid: {
                        color: gridColor
                    },
                    ticks: {
                        color: textcolor
                        }
                }
                ylimits[`y${i}`] = {
                    min: Math.min(...yValues[i]) - (Math.max(...yValues[i]) - Math.min(...yValues[i]))*0.1,
                    max: Math.max(...yValues[i]) + (Math.max(...yValues[i]) + Math.min(...yValues[i]))*0.1  
                }
                break;
            case "semi stacked":
                multiplescales[`y${i}`] = {
                    display: false,
                    title: {
                        display: true,
                        text: parameter[i],
                        color: textcolor 
                    },
                    grid: {
                        color: gridColor
                    },
                    ticks: {
                        color: textcolor
                        },
        
                    type: 'linear',
                    offset: true,
                    position: 'left',
                
                    //stack: FALSE,
                    min: Math.min(...yValues[i]) - i*(Math.max(...yValues[i]) - Math.min(...yValues[i])),
                    max: Math.max(...yValues[i]) + (yValues.length -1 -i)*(Math.max(...yValues[i]) - Math.min(...yValues[i]))            
                    
                }
                ylimits[`y${i}`] = {
                    min: Math.min(...yValues[i]) - i*(Math.max(...yValues[i]) - Math.min(...yValues[i])),  
                    max: Math.max(...yValues[i]) + (yValues.length -1 -i)*(Math.max(...yValues[i]) - Math.min(...yValues[i]))
                }
                break;
            case "not stacked": 
                multiplescales[`y${i}`] = {
                    display: false,
                    title: {
                        display: true,
                        text: parameter[i],  
                        color: textcolor
                    },
                    grid: {
                        color: gridColor
                    },
                    ticks: {
                        color: textcolor
                        },
        
                    type: 'linear',
                    offset: true,
                    position: 'left',

                
                    //stack: FALSE,
                    min: Math.min(...yValues[i]) - (Math.max(...yValues[i]) - Math.min(...yValues[i]))*0.1,
                    max: Math.max(...yValues[i]) + (Math.max(...yValues[i]) - Math.min(...yValues[i]))*0.1            
                }
                ylimits[`y${i}`] = {
                    min: Math.min(...yValues[i]) - (Math.max(...yValues[i]) - Math.min(...yValues[i]))*0.1,
                    max: Math.max(...yValues[i]) + (Math.max(...yValues[i]) - Math.min(...yValues[i]))*0.1 
                }
        }
    }

    myChart = new Chart(simpleChart, {
        type: 'line',
        data: {
            labels: tValues,
            datasets: dsets,    
        },
        options: {
            responsive: true,
            animation: false,
            scales : multiplescales,
            plugins: {
                legend: {
                    labels: {color: textcolor}
                },
                tooltip: {
                    enabled: true
                },
                
                zoom: {
                    limits: ylimits,
                    pan: {
                        enabled: true,
                        onPanStart({chart, point}) {
                            const area = chart.chartArea;
                            const w10 = area.width * 0.1;
                            const h10 = area.height * 0.1;
                            if (point.x < area.left + w10 || point.x > area.right - w10
                                || point.y < area.top + h10 || point.y > area.bottom - h10) {
                                return false; // abort
                            }
                        },
                        mode: settings.pan_mode, // Enable panning on the x-axis
                        threshold: settings.pan_treshold, // Minimum distance to start panning (useful for touch devices)
                        speed: settings.pan_speed, // Adjust pan speed (lower = slower)
                    },
                    zoom: {
                        wheel: {
                            enabled: true,
                            speed: settings.zoom_wheel_speed
                        },
                        pinch: {
                            enabled: true, // Enable pinch zooming on touch devices
                            speed: settings.zoom_pinch_speed
                        },
                        mode: settings.zoom_mode, // Zoom in/out on the x-axis                        
                    }
                }
            }       
        },        
    });
}

//used to get rid of special characters in string so that they can be used in methods. 
function escapeRegex(string) {
    return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'); // Escape regex special characters
}
