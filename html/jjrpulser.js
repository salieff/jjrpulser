var base_prefix = window.location.origin + window.location.pathname.substring(0, window.location.pathname.lastIndexOf('/') + 1);

function initJJRImages()
{
    var allElements = document.getElementsByTagName('img');

    for (var i = 0, n = allElements.length; i < n; i++)
    {
        if ('jjrtype' in allElements[i].dataset)
        {
            var imgSrc2 = base_prefix + 'mysql_jjrpulser2_' + allElements[i].dataset.jjrtype + '.png';
            allElements[i].src = imgSrc2;

            // Preload second image
            var imgSrc = base_prefix + 'mysql_jjrpulser_' + allElements[i].dataset.jjrtype + '.png';
            var img = new Image();
            img.src = imgSrc;
        }
    }
}

function changeJJRImage(img)
{
    var img1 = base_prefix + 'mysql_jjrpulser_' + img.dataset.jjrtype + '.png';
    var img2 = base_prefix + 'mysql_jjrpulser2_' + img.dataset.jjrtype + '.png';

    img.src = (img.src === img1) ? img2 : img1;
}
