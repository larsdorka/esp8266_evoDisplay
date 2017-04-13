$(function () {
    $('#myTabs a[href="#static"]').click(function (e) {
        e.preventDefault();
        $(this).tab('show');
    });
});

$(function () {
    $('#myTabs a[href="#apidoc"]').click(function (e) {
        e.preventDefault();
        $(this).tab('show');
        $.ajax({
            url: "api",
            dataType: 'text',
            async: true,
            statusCode: {
                200: function (response) {
                    $("#apidoctextarea").val(response);
                }
            },
            error: function () {
                $("#apidoctextarea").val('error');
            }
        });
    });
});

$(function () {
    $('#myTabs a[href="#led"]').click(function (e) {
        e.preventDefault();
        $(this).tab('show');
    });
});

$(function () {
    $('#colorModeSelector').change(function (e) {
        if (e.currentTarget.value != 'content')
            $('#colorContentInput').attr('disabled', 'true');
        else
            $('#colorContentInput').removeAttr('disabled');
        createPreview();
    });
});

$(function () {
    $('#textContentInput').change(createPreview);
});

$(function () {
    $('#colorContentInput').change(createPreview);
});

function createPreview() {
    $('#previewBox').empty();
    var textChars = $('#textContentInput').val().split('');
    var colorChars = $('#colorContentInput').val().split('');

    if ($('#colorModeSelector').val() == 'content') {
        if (colorChars.length < textChars.length) {
            $('#btn_send').attr('disabled', 'true');
            $('#errorText').text('Invalid: color string to short');
        }
        for (var c = 0; c < colorChars.length; c++) {
            switch (colorChars[c].toUpperCase()) {
                case 'R':
                    break;
                case 'O':
                    break;
                case 'G':
                    break;
                default:
                    $('#btn_send').attr('disabled', 'true');
                    $('#errorText').text('Invalid: color string may only contain "R, O, G"');
                    break;
            }
        }
    }
    else {
        $('#btn_send').removeAttr('disabled');
        $('#errorText').text('');
    }

    for (var i = 0; i < textChars.length; i++) {
        var appendString = '<div class="';
        switch ($('#colorModeSelector').val()) {
            case 'content':
                if (colorChars[i]) {
                    switch (colorChars[i].toUpperCase()) {
                        case 'R':
                            appendString += 'style-red">';
                            break;
                        case 'O':
                            appendString += 'style-orange">';
                            break;
                        case 'G':
                            appendString += 'style-green">';
                            break;
                        default:
                            appendString += 'style-black">';
                            break;
                    }
                }
                else {
                    appendString += 'style-black">';
                }
                break;
            case 'red':
                appendString += 'style-red">';
                break;
            case 'orange':
                appendString += 'style-orange">';
                break;
            case 'green':
                appendString += 'style-green">';
                break;
            default:
                appendString += 'style-black">';
                break;
        }
        appendString += textChars[i] + '</div>';
        $('#previewBox').append(appendString);
    }
}

function clearInput() {
    $('#previewBox').empty();
    $('#btn_send').removeAttr('disabled');
    $('#colorContentInput').removeAttr('disabled');
    $('#errorText').text('');
    $("#colorContentInput").val("");
    $("#colorModeSelector").val("content");
    $("#textContentInput").val("");
    $("#textModeSelector").val("0");
}

$(function () {
    $.ajaxSetup({"contentType": "text/json"});
});

$(function () {
    $("#btn_send").click(
        function () {
            var myObj = {};
            var myColor = {};
            var myText = {};
            myColor["content"] = $("#colorContentInput").val().toUpperCase();
            myColor["mode"] = $("#colorModeSelector").val();
            myText["content"] = $("#textContentInput").val();
            myText["mode"] = $("#textModeSelector").val();
            myObj["color"] = myColor;
            myObj["text"] = myText;
            var myJson = JSON.stringify(myObj);
            $.post("api/setdisplay", myJson, function () {
            });
        });
});

$(function () {
    $("#btn_clear").click(
        function () {
            clearInput();
            $.get("api/clear", function () {
            });
        });
});

$(function () {
    $("#btn_setled").click(
        function () {
            var params = "?chan=" + $("#channelSelect").val() + "&val=" + $("#valueInput").val();
            $.get("api/led" + params, function () {
            });
        });
});
