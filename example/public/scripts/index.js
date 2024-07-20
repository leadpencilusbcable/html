const tableHeadParamNames = [
    "name",
    "cr",
    "hp",
    "ac",
];

function onLoad()
{
    const thList = document.getElementById("monster-table").children[0].getElementsByTagName("th");

    for(let i = 0; i < thList.length; i++){
        const ele = thList[i];

        ele.addEventListener("click", () => {
            if(!tableHeadParamNames[i]) return;

            const imgChild = ele.children[0].children[1];

            if(imgChild.className === "rotateup"){
                processSort(tableHeadParamNames[i], i, false);
            }
            else{
                processSort(tableHeadParamNames[i], i, true);
            }
        });
    }
}

async function processSort(fieldName, fieldIndex, asc = true)
{
    const field = document.getElementById("thead" + fieldIndex);

    const imgChild = field.children[0].children[1];

    const table = document.getElementById("monster-table");

    imgChild.className = asc ? "rotateup" : "rotatedown";

    let sortField = fieldName;

    if(asc === false) sortField += "_dsc";

    const queryParams = { sort: sortField };
    const res = await fetchTable(queryParams);

    table.children[1].outerHTML = await res.text();

    const thList = document.getElementById("monster-table").children[0].getElementsByTagName("th");

    for(let i = 0; i < thList.length; i++){
        if(i !== fieldIndex){
            const imgChild = thList[i].children[0].children[1];
            imgChild.className = "";
        }
    }
}

async function fetchTable(queryParams = {})
{
    const searchParams = new URLSearchParams(queryParams);

    const res = await fetch("http://localhost:8000/monsters?" + searchParams.toString());
    return res;
}
